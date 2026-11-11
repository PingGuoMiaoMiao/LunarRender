# 2026-11 工具链兼容性检查与修复记录

本记录对应 MoonBit 开源大赛组委会 2026-11 的持续可用性维护提醒，检查日期 2026-11-11。

## 结论

LunarRender 在官方最新工具链上可以正常检查、测试、构建，并复现核心示例。
唯一需要修改的是 `formats/mmd` 中的弃用语法与黑盒测试写法；`renderer`、`backend/opengl`、
`platform/native`、`cmd/lunarrender` 的源码不需要修改。

## 验证环境

```text
moon    0.1.20261104 (940aa18 2026-11-04)  C:\Users\<user>\.moon\bin\moon.exe
moonc   v0.10.20+65efd5b93 (2026-11-05)    C:\Users\<user>\.moon\bin\moonc.exe
moonrun 0.1.20261104 (940aa18 2026-11-04)  C:\Users\<user>\.moon\bin\moonrun.exe

Feature flags enabled: rr_moon_mod,rr_moon_pkg
```

平台：Windows 11 x64、Visual Studio 2022（MSVC 19.41.34123.0）、CMake 4.4.2、
NVIDIA 驱动 610.88（OpenGL 3.3.0 Core）。

## 检查项与结果

| 检查项 | 命令 | 结果 |
| --- | --- | --- |
| 类型检查 | `moon check --target native` | `Finished. moon: ran 19 tasks ... (0 warnings, 0 errors)` |
| native 测试 | `moon test --target native` | `Total tests: 23, passed: 23, failed: 0` |
| 纯 MoonBit 包（wasm-gc） | `moon test --target wasm-gc` | `Total tests: 14, passed: 14, failed: 0` |
| Release 构建 | `moon build --target native --release cmd/lunarrender` | 通过，产物 `_build/native/release/build/cmd/lunarrender/lunarrender.exe` |
| 一键脚本 | `.\scripts\build.ps1` | 退出码 0（CMake + check + test + build 全流程） |
| 核心示例 | `lunarrender.exe --self-test` | 退出码 0，日志见下 |

核心示例日志（`--self-test`）：

```text
LunarRender: textured cube example started
LunarRender platform window: 960x540
LunarRender OpenGL backend: 3.3.0 NVIDIA 610.88
LunarRender OpenGL texture upload: key=1 size=8x8
LunarRender OpenGL material upload: key=1 texture=1 alpha=0
LunarRender OpenGL mesh upload: key=1 vertices=36 material=1
LunarRender OpenGL backend: destroyed
LunarRender platform window: destroyed
```

## 源码修改

新工具链把 `derive(...)` 标记为弃用语法（warning 0027 `deprecated_syntax`），并要求显式
声明 trait 方法，避免隐式提升（warning 0079 `implicit_impl_as_method`）；黑盒测试也不再
隐式导入被测包（warning 0025 `test_unqualified_package`）。

`formats/mmd/mmd.mbt`：

```diff
-pub(all) struct PmxMaterial {
+#derive(Debug)
+pub(all) struct PmxMaterial {
   ...
-} derive(Debug)
+}
+
+pub extend PmxMaterial with @debug.Debug::{to_repr}
```

`PmxModel` 同样处理。`formats/mmd/moon.pkg` 增加：

```text
import {
  "moonbitlang/core/debug",
}
```

`formats/mmd/mmd_test.mbt` 中 4 处 `parse(...)` 改为 `@mmd.parse(...)`。

修改后 `moon check --target native` 为 0 warning；`Int` 仍作为 `Int32` 的别名保留，
因此没有做全仓 `Int` → `Int32` 的批量重写。

## CI

新增 `.github/workflows/ci.yml`：

- `native-windows`（windows-latest）：安装最新 MoonBit 工具链 → CMake 配置并构建
  GLFW/GLAD 静态库 → `moon check` / `moon test` / Release 构建 → 校验产物存在且
  工作区没有意外改动。
- `pure-libs-wasm-gc`（ubuntu-latest）：对纯 MoonBit 包执行 `moon check --target wasm-gc`
  与 `moon test --target wasm-gc`。

CI 不运行窗口示例：GitHub 托管运行器没有可用的 OpenGL 3.3 上下文，窗口验证在本地
按 README 的“运行”一节手工执行。

## 已知限制

- 只支持 Windows x64 原生后端；`backend/opengl` 与 `platform/native` 声明
  `supported_targets = "native"`。
- 非原生目标上只有 `renderer`、`formats/mmd` 参与检查与测试。
- GLFW 3.4 的 `cmake_minimum_required(VERSION 3.4...3.28)` 在 CMake 4.4.2 上实测可以
  配置和构建；如果后续 CMake 版本恢复对 `< 3.5` 的硬性拒绝，可以在配置时加
  `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` 作为过渡。
- 本机曾出现 MSBuild 报 `MSB6001 ... 无效的环境变量名 no_proxy`：原因是进程环境里同时
  存在 `NO_PROXY` 与 `no_proxy`（大小写不同的重复项），属于本地代理工具/终端环境问题，
  与仓库无关。清理重复项后 CMake/VS 构建正常。
