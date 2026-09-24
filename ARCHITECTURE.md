# LunarRender Architecture

## 1. 角色

LunarRender 是 MoonMC 的 Windows native/OpenGL 客户端，不是世界逻辑仓库。它通过 Mooncakes 依赖 `PingGuoMiaoMiao/MoonMC@0.1.0`，将核心生成的网格和 `RenderSnapshot` 转换为 GPU 绘制。

## 2. 目录

```text
cmd/lunarrender       参数、输入、主循环和客户端编排
renderer              Renderer 边界、GPU 上传和绘制顺序
platform/native       GLFW、GLAD、WIC、文件、存档和 OpenGL C FFI
core/mmd              可选 PMX 解析包，不参与默认主循环
assets/textures       默认材质和玩家皮肤输入
resourcepacks         运行时资源包 manifest
third_party/glfw      GLFW 3.4 源码
third_party/glad      GLAD 生成器/源码
third_party/glad-generated  OpenGL loader 2.0.8 生成文件
scripts               Windows 构建和资源图集脚本
```

## 3. 所有权边界

| 内容 | 所有者 |
|---|---|
| 世界、区块、物理、射线和命令值 | MoonMC |
| ChunkMeshData、PlayerMeshData、RenderSnapshot | MoonMC 生成，LunarRender 消费 |
| GLFW window、键鼠状态和时间 | LunarRender |
| `resourcepacks/<name>/pack.json` 文件读取与 WIC 解码 | LunarRender |
| shader、VAO、VBO、纹理和 GPU 缓存 | LunarRender |
| 存档 JSON/二进制格式 | MoonMC；文件读写由 LunarRender 驱动 |
| 私有 MMD 解析与绘制 | LunarRender 可选后端 |

## 4. Renderer 接口

```moonbit
pub fn Renderer::upload_chunk(
  self : Renderer,
  position : @world.ChunkPos,
  mesh : @mesh.ChunkMeshData,
) -> Bool

pub fn Renderer::remove_chunk(
  self : Renderer,
  position : @world.ChunkPos,
) -> Unit

pub fn Renderer::upload_player(
  self : Renderer,
  mesh : @mesh.PlayerMeshData,
) -> Bool

pub fn Renderer::draw_frame(
  self : Renderer,
  snapshot : @render.RenderSnapshot,
) -> Unit

pub fn Renderer::shutdown(
  self : Renderer,
) -> Unit
```

`upload_chunk` 按 `ChunkPos` 管理 GPU 区块槽位；`upload_player` 替换当前玩家或第一人称手臂网格；`draw_frame` 消费矩阵、选中方块、玩家显示意图和 HUD；`shutdown` 删除全部 OpenGL 资源。

## 5. 主循环

```text
输入采集
→ 相机更新
→ MoonMC 玩家物理
→ 射线检测和方块编辑
→ 区块流式调度
→ dirty 区块网格重建与上传
→ PlayerMeshData 上传
→ RenderSnapshot 构造
→ Renderer::draw_frame
→ 交换缓冲
```

LunarRender 不直接生成世界规则，也不在 C 中解析 MoonBit 世界结构体。只通过显式 FFI 参数传递连续数组、标量和路径字符串。

## 6. 资源和可选 MMD

默认材质图集由纯 MoonBit CPU 逻辑生成，再由 `gl_replace_texture_atlas` 上传。MMD 默认关闭，公共仓库只保留无私有素材的 PMX 解析包，不保存私有 PMX、VMD、贴图或生成 C 文件。可选后端必须通过独立构建开关和本地路径准备，不改变体素 Renderer 接口。
