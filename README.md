# LunarRender

LunarRender 是一个使用 MoonBit 构建的跨后端实时渲染运行时。它把通用渲染数据、窗口平台和图形 API 后端分开，供游戏、可视化工具和模型预览程序使用。

当前首个后端是 Windows native、GLFW 3.4、GLAD 2.0.8 和 OpenGL 3.3 Core。

## 当前内容

- 纯 MoonBit `renderer` 网格/帧数据接口。
- Windows GLFW 窗口、输入、时间和文件/图片读取。
- OpenGL 3.3 Core Shader、VAO、VBO、网格上传和销毁。
- 由 MoonBit 生成 8×8 RGBA 棋盘纹理、材质和 36 顶点旋转立方体示例程序 `cmd/lunarrender`。
- 独立的可选 `formats/mmd` PMX 解析模块。

## 兼容性与验证

[![CI](https://github.com/PingGuoMiaoMiao/LunarRender/actions/workflows/ci.yml/badge.svg)](https://github.com/PingGuoMiaoMiao/LunarRender/actions/workflows/ci.yml)

2026-11-11 使用官方最新工具链完成兼容性检查，`moon version --all` 记录如下：

```text
moon    0.1.20261104 (940aa18 2026-11-04)
moonc   v0.10.20+65efd5b93 (2026-11-05)
moonrun 0.1.20261104 (940aa18 2026-11-04)
```

| 检查项 | 命令 | 结果 |
| --- | --- | --- |
| 类型检查 | `moon check --target native` | 通过，0 warning |
| native 测试 | `moon test --target native` | 23/23 通过 |
| 纯 MoonBit 包（wasm-gc） | `moon test --target wasm-gc` | 14/14 通过 |
| Release 构建 | `moon build --target native --release cmd/lunarrender` | 通过 |
| 核心示例 | `lunarrender.exe --self-test` | 窗口 960×540、OpenGL 3.3.0，纹理/材质/网格上传与销毁日志完整 |

本次为适配新工具链所做的源码变更：

- `formats/mmd`：`derive(Debug)` 迁移为 `#derive(Debug)`，并按编译器提示补充
  `pub extend ... with @debug.Debug::{to_repr}`；`moon.pkg` 显式导入
  `moonbitlang/core/debug`。
- `formats/mmd/mmd_test.mbt`：黑盒测试改用 `@mmd.parse`（新工具链不再隐式导入本包）。

平台与版本限制：

- 运行示例（正常窗口或 `--self-test`）需要 Windows 10/11 x64、桌面会话和支持
  OpenGL 3.3 Core 的驱动；没有桌面/GPU 的 CI 只做构建与测试，不运行窗口。
- `renderer` 和 `formats/mmd` 是纯 MoonBit 包，可在 `wasm-gc` 等后端检查与测试；
  `backend/opengl`、`platform/native` 只声明 `supported_targets = "native"`。
- 第三方 GLFW 3.4 / GLAD 2.0.8 以源码形式存放在 `third_party/`，由 CMake 3.20+
  和 VS 2022 MSVC 构建；本机另外用 CMake 4.4.2 实测配置与构建通过。
- 只验证当前工具链（moon 0.1.20261104 / moonc v0.10.20）。升级工具链后请重新执行
  上表的检查项。

CI（`.github/workflows/ci.yml`）在每次 push / PR 上运行：`windows-latest` 安装最新
MoonBit 工具链、构建 GLFW/GLAD 静态库，并执行 `moon check`、`moon test` 和 Release
构建；`ubuntu-latest` 对纯 MoonBit 包执行 `wasm-gc` 检查与测试。

## 环境要求

- Windows 10/11 x64。
- Visual Studio 2022，安装 MSVC C/C++ 工作负载。
- CMake 3.20 或更高版本。
- MoonBit 工具链（2026-11 验证版本：moon 0.1.20261104 / moonc v0.10.20）。
- 支持 OpenGL 3.3 Core 的显卡驱动。

## 安装

1. 安装 MoonBit 工具链，并确认 `moon --version` 可以在 PowerShell 中运行。
2. 使用 Visual Studio Installer 安装 Visual Studio 2022 的 MSVC C/C++ 工作负载。
3. 安装 CMake 3.20 或更高版本，并确认 `cmake --version` 可以运行。
4. 克隆本仓库并进入项目根目录：

```powershell
git clone https://github.com/PingGuoMiaoMiao/LunarRender.git
cd LunarRender
```

5. 按下面的“构建”步骤生成 GLFW、GLAD 和 LunarRender native 程序。

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

构建产物路径会根据 MoonBit 是否处于临时 workspace 自动变化。使用解析脚本获取唯一的 Release 可执行文件：

```powershell
$exe = .\scripts\resolve-runtime.ps1
& $exe
```

## 运行

```powershell
$exe = .\scripts\resolve-runtime.ps1
& $exe
```

示例窗口显示带程序生成棋盘材质的旋转立方体。按 `Esc` 退出。窗口变化使用 GLFW framebuffer 像素尺寸更新 `glViewport`。运行日志会记录窗口创建、OpenGL 后端版本、纹理/材质/网格上传和反向销毁顺序。

自动化验证可以使用 `--self-test`，窗口运行约一秒后按正常顺序退出：

```powershell
$exe = .\scripts\resolve-runtime.ps1
& $exe --self-test
```

## 公共接口

`renderer.MeshData` 第一版每个顶点使用 6 个 Float：

```text
position.xyz, uv.xy, shade
```

`renderer.FrameData` 使用 16 个 Float 的列主序视图投影矩阵，并携带当前 framebuffer 的正整数宽高。

渲染资源使用独立的 `MeshHandle`、`TextureHandle` 和 `MaterialHandle`。`TextureData` 固定为严格的 RGBA8 字节，`MaterialData` 可引用纹理并声明 `Opaque` 或 `Blend`。OpenGL 后端的 `Renderer` 负责校验引用、上传、删除和绘制资源槽位。

## 性能

性能特征、测量方法和优化记录见 [docs/performance.md](docs/performance.md)。
2026-11 这一轮算法优化的实测结果（同机、`moon bench --release`、MSVC `/O2`）：

| 路径 | 优化前 | 优化后 | 倍数 |
| --- | --- | --- | --- |
| 一帧的 CPU 提交开销（256 个网格） | 43.9 µs | 1.79 µs | 24.5× |
| 句柄 key → 资源槽位查找 | 133.6 ns | 2.62 ns | 51× |
| 每帧视图投影矩阵 | 1.04 µs | 228 ns | 4.5× |

复现：

```powershell
moon bench --target native --release -p PingGuoMiaoMiao/LunarRender/cmd/lunarrender

cl /nologo /std:c11 /O2 /Fe:tools\resource_table_bench.exe tools\resource_table_bench.c
tools\resource_table_bench.exe
```

这些是 CPU 侧数字。示例本身受垂直同步与 GPU 限制，上表的意义是：网格/材质/纹理
数量增长时这些固定成本不再线性放大。

## 目录

```text
renderer/              通用纯 MoonBit 数据
backend/opengl/        OpenGL 3.3 Core 后端（含资源表与渲染路径）
platform/native/       Windows GLFW 平台层
formats/mmd/           可选 PMX 格式解析
cmd/lunarrender/       旋转纹理立方体示例
third_party/           GLFW、GLAD 和本地构建目录
tools/                 C 侧资源表基准与自检
docs/                  设计和实施记录
scripts/               构建与运行脚本
```

## 申请材料

项目申请书草稿位于 [`PROJECT_PROPOSAL.md`](PROJECT_PROPOSAL.md)。其中没有填写申请人、指导教师、经费、院系或学校等未提供的信息。

## 维护记录

- [docs/maintenance-2026-11.md](docs/maintenance-2026-11.md) — 2026-11 工具链兼容性检查与修复记录（完整命令与输出）。
- [docs/performance.md](docs/performance.md) — 性能特征、测量方法、本轮算法优化与后续优先级。

## 第三方项目参考

- [bgfx](https://github.com/bkaradzic/bgfx)：公共渲染 API 与多个后端分离。
- [sokol](https://github.com/floooh/sokol)：轻量图形层与应用/窗口层边界。
- [wgpu](https://github.com/gfx-rs/wgpu)：核心接口、硬件抽象和具体后端的分层。
- [ClassiCube](https://github.com/ClassiCube/ClassiCube)：小型 Minecraft 风格客户端的工程参考。
- [MoonBit FFI 文档](https://docs.moonbitlang.com/en/latest/language/ffi.html)。

## 许可证

本项目使用 [MIT License](LICENSE)。
