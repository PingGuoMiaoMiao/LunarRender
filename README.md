# LunarRender

LunarRender 是一个使用 MoonBit 构建的跨后端实时渲染运行时。它把通用渲染数据、窗口平台和图形 API 后端分开，供游戏、可视化工具和模型预览程序使用。

当前首个后端是 Windows native、GLFW 3.4、GLAD 2.0.8 和 OpenGL 3.3 Core。OpenGL 是实现方式，不是 LunarRender 的项目边界。

## 当前内容

- 纯 MoonBit `renderer` 网格/帧数据接口。
- Windows GLFW 窗口、输入、时间和文件/图片读取。
- OpenGL 3.3 Core Shader、VAO、VBO、网格上传和销毁。
- 由 MoonBit 生成 8×8 RGBA 棋盘纹理、材质和 36 顶点旋转立方体示例程序 `cmd/lunarrender`。
- 独立的可选 `formats/mmd` PMX 解析模块。

Minecraft 的世界、玩家、Steve、重力、碰撞、跳跃、方块编辑、HUD、物品栏、命令和存档属于 MoonMC 或其他游戏组合层，不属于 LunarRender 默认运行时。

## 环境要求

- Windows 10/11 x64。
- Visual Studio 2022，安装 MSVC C/C++ 工作负载。
- CMake 3.20 或更高版本。
- MoonBit 工具链。
- 支持 OpenGL 3.3 Core 的显卡驱动。

## 构建

在仓库根目录执行：

```powershell
cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
moon check --target native
moon test --target native
moon build --target native --release cmd/lunarrender
```

也可以使用：

```powershell
.\scripts\build.ps1
```

## 运行

```powershell
.\_build\native\release\build\cmd\lunarrender\lunarrender.exe
```

示例窗口显示带程序生成棋盘材质的旋转立方体。按 `Esc` 退出。窗口变化使用 GLFW framebuffer 像素尺寸更新 `glViewport`。运行日志会记录窗口创建、OpenGL 后端版本、纹理/材质/网格上传和反向销毁顺序。

自动化验证可以使用 `--self-test`，窗口运行约一秒后按正常顺序退出：

```powershell
.\_build\native\release\build\cmd\lunarrender\lunarrender.exe --self-test
```

## 公共接口

`renderer.MeshData` 第一版每个顶点使用 6 个 Float：

```text
position.xyz, uv.xy, shade
```

`renderer.FrameData` 使用 16 个 Float 的列主序视图投影矩阵，并携带当前 framebuffer 的正整数宽高。

渲染资源使用独立的 `MeshHandle`、`TextureHandle` 和 `MaterialHandle`。`TextureData` 固定为严格的 RGBA8 字节，`MaterialData` 可引用纹理并声明 `Opaque` 或 `Blend`。OpenGL 后端的 `Renderer` 负责校验引用、上传、删除和绘制资源槽位。

## 目录

```text
renderer/              通用纯 MoonBit 数据
backend/opengl/        OpenGL 3.3 Core 后端
platform/native/       Windows GLFW 平台层
formats/mmd/           可选 PMX 格式解析
cmd/lunarrender/       旋转纹理立方体示例
third_party/           GLFW、GLAD 和本地构建目录
docs/                  设计和实施记录
scripts/               构建与运行脚本
```

## 申请材料

项目申请书草稿位于 [`PROJECT_PROPOSAL.md`](PROJECT_PROPOSAL.md)。其中没有填写申请人、指导教师、经费、院系或学校等未提供的信息。

## 第三方项目参考

- [bgfx](https://github.com/bkaradzic/bgfx)：公共渲染 API 与多个后端分离。
- [sokol](https://github.com/floooh/sokol)：轻量图形层与应用/窗口层边界。
- [wgpu](https://github.com/gfx-rs/wgpu)：核心接口、硬件抽象和具体后端的分层。
- [ClassiCube](https://github.com/ClassiCube/ClassiCube)：小型 Minecraft 风格客户端的工程参考。
- [MoonBit FFI 文档](https://docs.moonbitlang.com/en/latest/language/ffi.html)。

## 许可证

本项目使用 [MIT License](LICENSE)。
