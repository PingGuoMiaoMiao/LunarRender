# LunarRender Agent Guide

## 项目身份

- 模块名固定为 `PingGuoMiaoMiao/LunarRender`，版本为 `0.1.0`。
- 本地项目路径固定为 `C:\Users\chen\Documents\ChatGPT\9月moonbit\LunarRender`。
- 远程仓库固定为 `https://github.com/PingGuoMiaoMiao/LunarRender.git`。
- 这是 Windows native 客户端，使用 MSVC、CMake、GLFW `3.4`、GLAD `2.0.8` 和 OpenGL `3.3 Core`。

## 所有权

- LunarRender 拥有 GLFW window、输入、时间、文件读取、WIC 图片解码、资源包上传、OpenGL shader、VAO/VBO、纹理、HUD 和绘制生命周期。
- MoonMC 拥有世界、玩家逻辑、区块、物理、射线、网格数据、命令、存档编码和渲染快照。
- LunarRender 只消费 `PingGuoMiaoMiao/MoonMC@0.1.0` 的纯 MoonBit数据，不把 OpenGL 句柄或 C 指针传入 MoonMC。
- MMD/PMX/VMD 是可选客户端后端。私有文件只通过本地准备脚本传入，不能进入默认 Git 产物。

## 实现规则

- 使用 MoonMC 中实际存在的包、字段、函数和接口；禁止猜测标识符的大小写、格式或结构。
- 变更 FFI 时同步修改 `platform/native/platform.mbt`、对应 C stub、`moon.pkg`、`FFI_BINDINGS.md` 和测试/日志入口。
- 所有 C FFI 符号统一使用 `lunarrender_*`；LunarRender 中不得保留旧项目的 C FFI 符号。
- C 侧不暴露 GLFW/OpenGL 指针和 MoonBit 结构体内部布局。所有资源都必须有销毁路径。
- 默认构建不能要求 PMX、VMD、私有纹理或生成 MMD C 文件。
- 不读取、保存、输出或请求 GitHub、Mooncakes 或其他凭据。

## 生命周期

```text
window_create
→ gl_init
→ shader/VAO/VBO/texture 创建
→ MoonMC 网格上传
→ 每帧输入、快照和绘制
→ gl_destroy_renderer
→ window_destroy
```

window 销毁后不得调用 OpenGL。初始化失败、shader 失败、纹理失败、网格上传失败和文件错误必须保留实际阶段信息。

## 固定验证

```powershell
cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
moon install
moon check --target native
moon test --target native
moon build --target native --release cmd/lunarrender
```

没有 Mooncakes 发布版本时，使用共同父目录的临时 `moon.work` 联调；不要提交 `moon.work`。不要自动执行 `moon publish` 或 `git push`。
