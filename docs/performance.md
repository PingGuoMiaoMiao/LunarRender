# 性能分析与优化记录（2026-11）

本记录说明 LunarRender 当前的性能特征、这一轮做的算法优化，以及下一步优先级。
所有数字都可以在仓库内复现，命令见每节。

## 仓库结构变化

2026-11 起仓库拆成两个 MoonBit 模块：核心模块在仓库根目录（`renderer/`、`formats/mmd/`），Windows native 后端在 `native/`（`native/backend/opengl/`、`native/platform/native/`、`examples/`、`tools/`）。本文的数字是拆分前在单体结构下测得的，命令路径已按新结构更新。

## 结论摘要

| 路径 | 优化前 | 优化后 | 倍数 |
| --- | --- | --- | --- |
| 一帧的 CPU 提交开销（256 个网格，C 侧） | 43.9 µs | 1.79 µs | **24.5×** |
| 句柄 key → 资源槽位查找（C 侧） | 133.6 ns | 2.62 ns | **51×** |
| 每帧视图投影矩阵（`demo_frame`，MoonBit） | 1.04 µs | 228 ns | **4.5×** |
| 4×4 矩阵乘法（`mat4_mul`） | 206 ns | 134 ns | 1.5× |
| 每帧堆分配次数（`demo_frame`） | 7 | 1 | 7× |

测量环境：Windows 11 x64、moon 0.1.20261104 / moonc v0.10.20、MSVC 19.41 `/O2`、
`moon bench --release`、NVIDIA 驱动 610.88。

> 这些数字都是 **CPU 侧** 的开销。示例只有一个 36 顶点立方体，真正限制帧率的是
> 垂直同步和 GPU；上表的意义在于：当场景里的网格、材质、纹理数量涨上去时，
> 这些开销不会线性放大。

## 怎么复现

```powershell
# MoonBit 侧：帧矩阵与网格构建（在 native 目录）
cd native
moon bench --target native --release -p PingGuoMiaoMiao/LunarRenderNative/examples/textured_cube

# C 侧：资源查找与每帧遍历（不需要 GPU / OpenGL 上下文；在仓库根目录）
cl /nologo /std:c11 /O2 /Fe:native\tools\resource_table_bench.exe native\tools\resource_table_bench.c
native\tools\resource_table_bench.exe
```

`native/tools/resource_table_bench.c` 同时做两件事：先用随机增删和参考实现比对，
校验表的不变式（`--check` 只跑校验），再对「旧的全表扫描」与「新的哈希表 +
稠密活跃表」计时。它直接 include 了后端使用的那份 `resource_table.h`，
测的就是实际实现，不是复制品。

## 优化 1：后端资源查找从 O(容量) 变成 O(1)

**问题.** `native/backend/opengl/opengl.c` 用三个固定容量 256 的数组保存网格、材质、纹理。
查找资源是线性扫描整张表，绘制一帧时要先扫 256 个槽位，再为每个可见网格扫一遍
材质、扫一遍纹理。256 个网格的一帧大约是 13 万次整数比较，复杂度是
`O(容量 × 网格数)`，与场景规模无关的固定成本很重。

**改法.** 新增 `native/backend/opengl/resource_table.h`：

- key → 槽位用开放寻址哈希表（512 桶、≤256 个资源），查找期望 O(1)；
- 维护一份稠密活跃列表 `active[]`，绘制循环只遍历真实存在的网格，复杂度从
  `O(容量)` 变为 `O(活跃网格)`；
- 槽位分配用空闲栈，上传/删除也是 O(1)；删除用墓碑标记，墓碑超过阈值时重建索引。

**保留的行为.** C ABI 不变、日志行不变、槽位上限不变；重复上传同一个 key 现在
复用已有槽位。

## 优化 2：重复上传网格不再重建 VAO/VBO

旧实现每次上传都 `glGenVertexArrays` + `glGenBuffers` + 重新设置顶点属性，即使这个
句柄已经存在。现在沿用已有 VAO/VBO，只做 `glBufferData` 重写数据，属性布局保持
不变。对每一帧都在改网格数据的场景（动画、体素、粒子），这条路径能省掉每帧的
GPU 对象创建。

`--self-test` 会重复上传同一个网格一次，专门覆盖这条复用路径，日志里会出现两行
`mesh upload` 和一行 `mesh re-upload reuse ok`。

## 优化 3：帧矩阵从 7 次分配、3 次乘法降到 1 次分配、1 次乘法

`demo_frame` 原来每帧构建 `rotation_x`、`rotation_y`、`mat4_mul`×3、`translation`、
`perspective`，一共 7 个 16 元素数组，其中 6 个是临时对象，而且透视矩阵只和视口
宽高比有关，却每帧重算。

现在：

- 旋转与平移按解析式直接写进模型视图矩阵（`Ry · Rx` 的 3×3 展开、平移列直接写
  `-3.2`），省掉两次 4×4 乘法；
- 透视矩阵按宽高比缓存，只有窗口尺寸变化才重算；
- 每帧只分配最终返回的 16 个 float；
- `mat4_mul` 把内层 `k` 循环展开，每列只读一次 `right`，减少索引计算和边界检查。

正确性由白盒测试 `demo_frame composes the same matrix as the matrix helpers` 保证：
它用原来的矩阵辅助函数（现在只存在于 `matrix_reference_wbtest.mbt`）重算一遍，
逐元素比较到 1e-6。另一个测试检查宽高比变化时投影确实改变。

## 优化 4：PMX 解析器不再「先增长数组再复制」

`formats/mmd` 的 `parse` 原来用可增长 `Array` 逐项 push，最后再整份复制成
`FixedArray`；`render_vertex_count()` 更夸张：为了返回一个计数，会把整个 12
浮点/顶点的渲染数组建出来再除以 12。顶点数 200 万、索引 600 万的上限下，这个
「只数一下」的调用可能触发几百 MB 的临时分配。

现在：

- `parse` 用文件里已知的 `vertex_count` / `index_count` 一次性分配精确大小，
  按下标写入，没有增长与复制；
- 新增 `render_vertex_slots`，只扫描索引区间做校验和计数，`render_vertex_count`
  与 `render_vertices` 的预分配共用它；
- 增加了两个廉价的前置校验（顶点数、索引字节数必须能被剩余文件长度容纳），
  避免畸形文件用伪造的计数诱导大块分配。

## 还没做、但收益更大的方向（按优先级）

这些都需要改公共接口或新增能力，因此本轮没有动：

1. **绘制批处理与状态排序**：按材质/纹理排序提交，把 `glBindTexture` 和 uniform
   更新次数从「每个网格一次」降到「每种状态一次」。收益随场景规模线性增长，
   需要在 `FrameData`/`MeshData` 之外引入可排序的绘制列表。
2. **视锥剔除**：给 `MeshData` 增加 AABB，逐帧对视图投影矩阵做包围盒测试，
   完全跳过屏幕外网格。省的是 GPU 顶点与片元工作，不只是 CPU 提交。
3. **实例化绘制**：同一网格 + 不同变换用 `glDrawArraysInstanced` + 每实例属性，
   把 N 次 draw call 变成 1 次。需要把「每对象变换」纳入帧数据。
4. **动态缓冲区分流**：`GL_STATIC_DRAW` 适合静态网格；每帧变化的网格应使用
   `GL_DYNAMIC_DRAW` + `glBufferSubData` 做局部更新，避免整块重传。
5. **SIMD 矩阵/蒙皮**：MoonBit 工具链提供 `v128`，可以把矩阵乘法和顶点蒙皮向量化。
   `mat4_mul` 现在约 134 ns，是纯标量实现；向量化后预期还有 2–4 倍空间。
   需要先确认 `v128` 在 native 后端的可用性与打包方式。
6. **异步资源上传**：纹理解码（`native/platform/native/image_io.c`）目前在主线程同步执行，
   大贴图会卡帧；可以放到工作线程 + 上传队列。
7. **PMX 解析的增量指标**：解析器还可以跳过不参与渲染的材质/骨骼字段（目前
   `skip_string` 仍会逐字节验证 UTF-16），对超大模型能再省一部分。

## 测量注意事项

- C 侧基准只测 CPU 记账，不含驱动与 GPU 时间；请把它当作「固定成本是否随容量
  增长」的证据，而不是 FPS 预测。
- `moon bench` 的数字受机器负载影响，比较时应看同一台机器上的前后差值。
- 示例程序的帧率主要受垂直同步限制，`--self-test` 只验证功能与顺序，不做性能断言。
