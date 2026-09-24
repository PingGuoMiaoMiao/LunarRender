# LunarRender 通用渲染运行时设计

## 1. 目标

将 LunarRender 从“MoonMC 的 OpenGL 游戏客户端”重构为独立的跨后端渲染运行时。LunarRender 保留 Windows native、GLFW 和 OpenGL 作为首个可运行后端，但项目公共接口不再依赖 MoonMC，也不拥有 Minecraft 游戏规则。

MoonMC 继续负责世界、玩家、物理、方块交互、HUD、物品栏、命令、存档和游戏资源策略。未来由单独的游戏组合层把 MoonMC 的纯数据转换为 LunarRender 的通用网格和帧数据。

## 2. 已确认的边界

### LunarRender 拥有

- 窗口生命周期、输入设备、时间和平台事件。
- 通用渲染数据结构、GPU 资源生命周期和帧提交。
- 材质、纹理、网格、Shader、Buffer 和 Render Target 的运行时抽象。
- `backend/opengl` 的 OpenGL 3.3 Core 实现。
- 通用文件和图片读取接口。
- 可选模型格式导入器，包括独立的 MMD/PMX 解析模块。

### LunarRender 不拥有

- Minecraft 世界、区块、方块、玩家、重力、碰撞和跳跃。
- Steve 角色语义、第一/第三人称规则、方块破坏/放置。
- HUD、物品栏、命令行、存档规则和 Minecraft 资源包策略。
- `PingGuoMiaoMiao/MoonMC` 模块依赖。

## 3. 分层结构

```text
LunarRender
├─ renderer              通用渲染数据和 Renderer 公共接口
├─ backend/opengl        OpenGL 3.3 Core 后端
├─ platform/native       Windows 窗口、输入、时间和文件平台层
├─ formats/mmd           可选 PMX 解析与模型数据转换
├─ cmd/lunarrender       不包含游戏规则的最小渲染示例
└─ third_party           GLFW 3.4、GLAD 2.0.8
```

依赖方向固定为：

```text
游戏组合层 → MoonMC
游戏组合层 → LunarRender/renderer
LunarRender/backend/opengl → LunarRender/renderer
LunarRender 不依赖 MoonMC
```

## 4. 公共数据接口

`renderer` 只暴露与游戏无关的数据：

```moonbit
pub(all) struct MeshData {
  vertices : FixedArray[Float]
  vertex_count : Int
  floats_per_vertex : Int
}

pub(all) struct FrameData {
  view_projection : FixedArray[Float]
}
```

`MeshData` 的第一版固定要求 `floats_per_vertex == 6`，排列为位置 3 个、UV 2 个、明暗 1 个。这个限制属于当前 OpenGL 后端的已记录能力边界，不把 Minecraft 的区块、玩家或方块字段带入接口。后续增加颜色、法线、骨骼和实例数据时，通过新的顶点格式扩展，不修改 MoonMC 数据结构。

后端提供：

```moonbit
pub fn Renderer::new() -> Renderer?
pub fn Renderer::upload_mesh(self : Renderer, key : Int, mesh : @renderer.MeshData) -> Bool
pub fn Renderer::remove_mesh(self : Renderer, key : Int) -> Unit
pub fn Renderer::draw_frame(self : Renderer, frame : @renderer.FrameData) -> Unit
pub fn Renderer::shutdown(self : Renderer) -> Unit
```

当前 `key` 只表示渲染器资源槽位，不代表区块坐标、玩家实体或游戏物品。

## 5. 后端和 FFI

窗口 FFI 保留在 `platform/native`，但 OpenGL FFI 移到 `backend/opengl`。OpenGL C 符号统一使用 `lunarrender_opengl_*`，平台窗口符号统一使用 `lunarrender_platform_*`。顶层 `renderer` 包不得声明 `extern "C"`。

生命周期固定为：

```text
platform window_create
→ backend/opengl context_init
→ Shader/VAO/VBO/Texture 创建
→ upload_mesh
→ draw_frame
→ backend/opengl shutdown
→ platform window_destroy
```

错误必须带有阶段信息，至少区分窗口创建、OpenGL context、GLAD、Shader、Buffer、Texture、网格上传和销毁阶段。

## 6. 示例程序

`cmd/lunarrender` 只渲染一个静态示例网格，验证窗口、后端初始化、网格上传、矩阵提交和资源销毁。它不加载 MoonMC 世界，不生成地形，不显示 Steve，不提供物品栏、命令和方块编辑。

## 7. 可选 MMD

PMX 解析器移动到 `formats/mmd`，只输出通用模型数据。MMD 不进入默认示例主循环，不要求私有 PMX、VMD、贴图或生成 C 文件。骨骼动画播放、角色选择和游戏中的位置控制属于使用方，而不是 LunarRender 核心。

## 8. 验收

- `moon.mod` 不再导入 MoonMC。
- `renderer` 包不导入 MoonMC、平台包或 C FFI。
- OpenGL 实现只存在于 `backend/opengl`。
- 默认示例构建不包含 Minecraft 游戏逻辑和私有模型。
- renderer 纯逻辑测试覆盖网格校验、矩阵校验和资源槽位行为。
- Windows MSVC/CMake 构建和运行日志确认 OpenGL 后端能够初始化、上传一个示例网格并按顺序销毁。

