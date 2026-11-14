# LunarRender

LunarRender 是一个与后端无关的 MoonBit 渲染数据与格式模块。它定义可验证的网格、纹理、材质和帧数据，也提供纯 MoonBit 的 MMD/PMX 格式解析；它不创建窗口、不调用 OpenGL，也不包含游戏逻辑。

Windows OpenGL 实现位于同一仓库的 [`native`](native/README.md) 模块，包名为 `PingGuoMiaoMiao/LunarRenderNative`。

[![CI](https://github.com/PingGuoMiaoMiao/LunarRender/actions/workflows/ci.yml/badge.svg)](https://github.com/PingGuoMiaoMiao/LunarRender/actions/workflows/ci.yml)

## 兼容性与验证

2026-11 使用官方最新工具链验证，`moon version --all` 记录如下：

```text
moon    0.1.20261104 (940aa18 2026-11-04)
moonc   v0.10.20+65efd5b93 (2026-11-05)
moonrun 0.1.20261104 (940aa18 2026-11-04)
```

| 检查项 | 命令 | 结果 |
| --- | --- | --- |
| 类型检查 | `moon check --target native` | 通过，0 warning |
| 核心模块测试（native） | `moon test --target native` | 14/14 通过 |
| 核心模块测试（wasm-gc） | `moon test --target wasm-gc` | 14/14 通过 |
| native 模块（workspace，含核心包） | `cd native; moon test --target native` | 25/25 通过 |
| native 三个示例 | `native/scripts/self-test.ps1` | 全部通过 |

平台与版本限制：

- 核心模块只需要 MoonBit 工具链，可在 `native`、`wasm-gc` 等后端检查与测试。
- Windows 窗口、OpenGL 3.3 Core、MSVC 和 CMake 只属于 [`native`](native/README.md) 模块。
- 本仓库只验证上表列出的工具链版本；升级工具链后请重新执行这些检查。

CI（`.github/workflows/ci.yml`）在每次 push / PR 上运行：`ubuntu-latest` 检查并测试核心模块（native 与 wasm-gc），`windows-latest` 构建静态库、检查并测试 native 模块、构建三个示例并校验产物。

## 使用核心模块

发布版本为 `0.2.1`。在其他 MoonBit 项目中执行：

```powershell
moon add PingGuoMiaoMiao/LunarRender@0.2.1
```

然后在 `moon.pkg` 中导入 `PingGuoMiaoMiao/LunarRender/renderer` 或 `PingGuoMiaoMiao/LunarRender/formats/mmd`。核心模块只输出数据，具体绘制后端由消费者决定。

## 核心模块检查

```powershell
moon fmt --check
moon check --target native
moon test --target native
moon check --target wasm-gc
moon test --target wasm-gc
moon package --list
```

本地同时开发核心与 native 模块时，`native/moon.work`（只含 `.` 与 `..`，不提交）把两个模块连成 workspace；`native/scripts/build.ps1` 会在缺失时自动创建它。

## 性能

核心模块本身只有数据与解析，性能记录见 [docs/performance.md](docs/performance.md)。2026-11 的算法优化集中在 native 后端的资源查找与每帧矩阵路径：

| 路径 | 优化前 | 优化后 | 倍数 |
| --- | --- | --- | --- |
| 一帧的 CPU 提交开销（256 个网格） | 43.9 µs | 1.79 µs | 24.5× |
| 句柄 key → 资源槽位查找 | 133.6 ns | 2.62 ns | 51× |
| 每帧视图投影矩阵 | 1.04 µs | 228 ns | 4.5× |

## 目录

```text
renderer/       纯 MoonBit 渲染数据类型和验证
formats/mmd/    纯 MoonBit PMX/MMD 格式解析
native/         Windows native 模块、OpenGL 后端、示例和构建脚本
docs/           兼容性检查、性能记录和项目计划
scripts/        发布辅助脚本
```

## 边界

LunarRender 不拥有世界、玩家、重力、碰撞、物品栏、命令、存档或联网状态；这些属于游戏组合层。OpenGL 只是 `LunarRenderNative` 的首个后端，不是核心模块的公共边界。

## 维护记录

- [docs/maintenance-2026-11.md](docs/maintenance-2026-11.md) — 2026-11 工具链兼容性检查与修复记录。
- [docs/performance.md](docs/performance.md) — 性能特征、测量方法与本轮算法优化。

## 许可证

MIT，见 [`LICENSE`](LICENSE)。
