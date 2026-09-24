# LunarRender 协作与实现约定

## 项目定位

LunarRender 是一个使用 MoonBit 编写的跨后端实时渲染运行时。OpenGL 3.3 Core 是当前第一个后端，不是项目名称、公共接口或最终平台范围。

当前模块为 `PingGuoMiaoMiao/LunarRender@0.1.0`，默认目标为 Windows native。GLFW 3.4、GLAD 2.0.8、CMake 和 MSVC 只属于当前平台后端的构建实现。

## 当前阶段

当前阶段是“通用渲染运行时重构”:

- `renderer` 只保存通用网格和帧数据。
- `backend/opengl` 管理 OpenGL 3.3 Core 的 GPU 资源。
- `platform/native` 管理 GLFW 窗口、输入、事件和时间。
- `formats/mmd` 只提供可选 PMX 格式解析，不参与默认示例。
- `cmd/lunarrender` 只运行静态网格示例。
- Minecraft 世界、Steve、物理、HUD、物品栏、命令和存档不属于本仓库的默认运行时。

游戏组合层可以同时使用 MoonMC 和 LunarRender，但 LunarRender 不依赖 MoonMC。

## 工作流程

1. 先阅读 `PROJECT_PROPOSAL.md`、`ARCHITECTURE.md`、`FFI_BINDINGS.md` 和 `ROADMAP.md`。
2. 先写能够表达边界的测试，再实现接口。
3. 先在隔离 worktree 中修改，确认后再合并到 `main`。
4. 每次涉及接口、包边界或 FFI 所有权的变化，先更新文档。
5. 使用实际源码、包配置、构建输出和运行日志确认结论，不凭名称推断接口。

## 边界规则

- `renderer` 不得导入平台包、OpenGL 包、C FFI 或游戏包。
- `backend/opengl` 是唯一声明 `lunarrender_opengl_*` 的包。
- `platform/native` 是唯一声明 `lunarrender_platform_*` 的包。
- `cmd/lunarrender` 不得恢复 MoonMC 的世界、玩家、物理、HUD、物品栏、命令或存档调用。
- 私有 PMX、PMD、VMD、贴图、绝对路径和生成的模型 C 文件不得进入 Git。
- 不在 LunarRender 中复制 MoonMC 的游戏规则；需要游戏功能时，在外部组合层适配 `MeshData` 和 `FrameData`。

## 修改规则

- 不猜测路径、包名、字段名、C 符号或参数所有权；先读取对应文件和测试。
- 使用 `apply_patch` 编辑文本和源码。
- 不使用 `git reset --hard`、`git clean` 或覆盖式复制。
- 不修改 `C:\Users\chen\Desktop\02_学习资料\practice笔记\Moonbit\MoonMC` 的主工作区。
- 不在本流程中发布 Mooncakes；GitHub 推送只在完整验证后执行。

## 验证要求

纯 MoonBit 包：

```powershell
moon check --target native
moon test --target native
```

Windows 后端：

```powershell
cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
moon build --target native --release cmd/lunarrender
```

完成前必须检查：

- `git diff --check` 无输出。
- 测试、CMake、MoonBit native 构建均通过。
- 启动示例并保留窗口、OpenGL、网格上传和销毁日志。
- `git status --short --branch` 清楚说明是否还有未提交文件。
- 不把编译缓存、运行日志、私有资源或凭据提交到仓库。
