# LunarRender 架构

## 1. 定位

LunarRender 是渲染运行时，不是 Minecraft 游戏本体。它接收应用提交的通用网格和帧数据，负责窗口、后端、GPU 资源和绘制生命周期。

首个运行后端是 Windows native + GLFW 3.4 + GLAD 2.0.8 + OpenGL 3.3 Core。后端可以在公共 `renderer` 接口稳定后继续增加，不把 OpenGL 句柄泄露给上层。

## 2. 目录边界

```text
LunarRender/
├─ renderer/              纯 MoonBit 网格和帧数据
├─ backend/opengl/        OpenGL 3.3 Core 实现和 GPU 资源
├─ platform/native/       Windows 窗口、输入、时间、文件和图片读取
├─ formats/mmd/           可选 PMX 解析和通用模型数据
├─ cmd/lunarrender/       静态网格示例程序
├─ third_party/            GLFW、GLAD 和构建产物目录
├─ scripts/                构建与运行记录脚本
└─ docs/                  设计、计划和验收记录
```

## 3. 依赖方向

```text
外部游戏或应用
  ├─ MoonMC 游戏核心
  └─ LunarRender/renderer
       └─ LunarRender/backend/opengl
            └─ LunarRender/platform/native
```

LunarRender 不导入 `PingGuoMiaoMiao/MoonMC`。MoonMC 拥有世界、区块、方块、玩家、物理、碰撞、跳跃、HUD、物品栏、命令、存档和游戏资源策略。LunarRender 拥有渲染数据、窗口、GPU 资源和图形后端。

## 4. 公共渲染数据

`renderer.MeshData` 的第一版字段为：

- `vertices`：交错的 `FixedArray[Float]`。
- `vertex_count`：顶点数量。
- `floats_per_vertex`：固定为 `6`。

每个顶点排列为：

```text
position.x position.y position.z uv.u uv.v shade
```

`renderer.FrameData` 保存严格 16 个 Float 的列主序视图投影矩阵。公共层只校验数据完整性，不创建 VAO、VBO 或纹理。

## 5. OpenGL 后端

`backend/opengl` 创建和销毁 Shader、VAO、VBO，并通过整数资源槽位保存通用网格。资源键只属于渲染器，不解释为区块坐标、实体 ID 或物品槽位。

当前后端使用固定的 6 Float 顶点布局和无外部纹理的示例 Shader。后续增加材质、法线、骨骼或实例数据时，应增加明确的顶点格式，而不是把游戏字段写入 `MeshData`。

## 6. 平台层

`platform/native` 创建 OpenGL 上下文所在的 GLFW 窗口，并提供事件轮询、键鼠状态、时间、文件和图片读取。它不创建 GPU 网格，也不决定应用的游戏规则。

## 7. 默认数据流

```text
应用构造 MeshData / FrameData
→ platform 创建窗口和 OpenGL context
→ backend/opengl 初始化 Shader 和资源槽位
→ upload_mesh(key, mesh)
→ 每帧 poll_events
→ draw_frame(frame)
→ swap_buffers
→ remove_mesh / backend shutdown
→ window destroy
```

## 8. MMD 边界

`formats/mmd` 只负责 PMX 二进制解析和格式数据。模型选择、角色身份、动作、骨骼播放、摄像机视角和游戏位置由使用方负责。没有私有模型时，默认静态示例仍必须构建和运行。
