# LunarRenderNative

`PingGuoMiaoMiao/LunarRenderNative` 是 LunarRender 的 Windows native 图形模块。它把核心模块生成的渲染数据交给 GLFW、GLAD 和 OpenGL 3.3 Core，提供三个可运行示例和 CMake 构建入口。

核心模块是同一仓库根目录的 `PingGuoMiaoMiao/LunarRender`；本模块依赖它（`0.2.1`）。

## 环境

1. 安装 MoonBit，并确保 `moon` 在 PowerShell 的 `PATH` 中。
2. 安装 Visual Studio 2022，勾选 Desktop development with C++ 和 Windows SDK。
3. 安装 CMake 3.20 或更高版本，并确保 `cmake` 在 `PATH` 中。
4. 安装支持 OpenGL 3.3 Core 的显卡驱动。

## 构建

在 `native` 目录执行：

```powershell
.\scripts\build.ps1
```

脚本会：创建本地 `moon.work`（只含 `.` 与 `..`，不提交）、用 CMake 构建 GLFW/GLAD 静态库、执行 `moon check` / `moon test`，再构建三个示例并打印可执行文件路径。

也可以手动执行同样的步骤：

```powershell
# 本地联调：把核心模块和本模块连成 workspace（该文件不提交）
@'
members = [
  ".",
  "..",
]
'@ | Set-Content -Encoding utf8 moon.work

cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
moon fmt --check
moon check --target native
moon test --target native
moon build --target native --release examples/textured_cube
```

> `moon.work` 必须是无 BOM 的 UTF-8；`build.ps1` 会自动按此写入。

## 示例

| 示例 | 内容 |
| --- | --- |
| `examples/triangle` | 窗口、最小着色器和一个三角形 |
| `examples/textured_cube` | 程序生成棋盘纹理、材质、36 顶点旋转立方体 |
| `examples/multi_mesh` | 两个网格的上传、绘制与销毁 |

自动验证：

```powershell
.\scripts\self-test.ps1
```

它依次运行三个示例的 `--self-test`（窗口运行约一秒后按正常顺序退出），任一退出码非 0 即失败。日志应包含窗口创建、OpenGL 版本、纹理/材质/网格上传、backend 销毁和窗口销毁。

## 性能

后端的资源查找与每帧提交路径做过算法优化（哈希表 + 稠密活跃表、状态去重、VAO/VBO 复用），测量方法与数字见 [`docs/performance.md`](../docs/performance.md)。

`tools/resource_table_bench.c` 是不需要 GPU 的基准与自检工具：

```powershell
cl /nologo /std:c11 /O2 /Fe:tools\resource_table_bench.exe tools\resource_table_bench.c
tools\resource_table_bench.exe
```

## 发布

核心模块必须先发布（本模块依赖 `PingGuoMiaoMiao/LunarRender@0.2.1`），然后在仓库根目录执行：

```powershell
.\scripts\publish-native.ps1 -DryRun
.\scripts\publish-native.ps1
```

脚本只在发布期间临时移走根 `.moonignore`（它排除了 `native/`，否则打包会得到空包），完成后自动恢复。

当前版本 `0.2.0`。发布前不要提交临时 `moon.work`、`_build`、私有模型或生成文件。

## 限制

只支持 Windows native、MSVC 和 OpenGL 3.3 Core。它不实现 Minecraft 世界、玩家物理、HUD、物品栏、命令或联网，也不要求私有 MMD 资源才能构建示例。
