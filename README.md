# LunarRender

LunarRender 是 `PingGuoMiaoMiao/LunarRender@0.1.0`，面向 Windows native 的 OpenGL 3.3 Core 客户端。它消费 `PingGuoMiaoMiao/MoonMC@0.1.0` 的纯 MoonBit世界、玩家、网格和渲染快照数据。

## 当前范围

- GLFW `3.4` 窗口和输入。
- GLAD `2.0.8` 加载 OpenGL 3.3 Core。
- 区块网格、玩家网格、第一人称手臂、HUD、选中框和资源包图集上传。
- Windows WIC 图片解码、原子存档文件写入和本地资源读取。
- 第一/第三人称、重力/碰撞/跳跃、破坏/放置和九槽热键栏由 MoonMC 核心逻辑提供，客户端负责输入与编排。
- MMD 是可选后端；默认构建不要求私有 PMX、VMD、贴图或生成 C 文件。

## 环境

- Windows x64。
- MoonBit 工具链。
- Visual Studio 2022 MSVC C/C++ 工作负载和 Windows SDK。
- CMake。

## 构建

MoonMC `0.1.0` 发布到 Mooncakes 后：

```powershell
moon install
cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
moon check --target native
moon test --target native
moon build --target native --release cmd/lunarrender
```

如需本地联调，在共同父目录使用临时 `moon.work` 指向 MoonMC worktree；不要提交它。`scripts/build.ps1` 会执行资源图集生成、CMake 和 native 构建，不会要求私有 MMD 文件。

## 运行

```powershell
& .\_build\native\release\build\PingGuoMiaoMiao\LunarRender\cmd\lunarrender\lunarrender.exe --seed 0
```

操作：W/A/S/D 移动，鼠标转向，Space 跳跃，1–9 选择方块，鼠标左键破坏，右键放置，F7 切换资源包，F8 切换第一/第三人称，`/` 打开命令行，Escape 保存并退出。

## 目录

```text
cmd/lunarrender       客户端主循环
renderer              GPU Renderer
platform/native       GLFW/GLAD/OpenGL/WIC/文件 FFI
core/mmd              可选 PMX 解析包，不参与默认客户端主循环
assets/textures       默认方块和玩家皮肤
resourcepacks         pack.json
third_party           GLFW、GLAD
scripts               Windows 构建和图集脚本
```

私有 MMD 文件和生成文件必须留在被 `.gitignore` 忽略的本地路径中。
