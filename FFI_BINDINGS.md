# LunarRender FFI 与构建边界

## 1. 固定版本和工具链

- Windows native。
- MSVC / Visual Studio 2022。
- CMake 3.20 或更高版本。
- GLFW 3.4。
- GLAD 2.0.8 生成的 OpenGL loader。
- OpenGL 3.3 Core。

GLFW、GLAD 和 OpenGL 只通过 `platform/native` 与 `backend/opengl` 使用；`renderer` 不出现 C FFI。

## 2. MoonBit 声明归属

平台声明位于：

```text
platform/native/platform.mbt
platform/native/moon.pkg
```

平台 C 符号统一为：

```text
lunarrender_platform_window_*
```

OpenGL 声明位于：

```text
backend/opengl/opengl.mbt
backend/opengl/moon.pkg
```

OpenGL C 符号统一为：

```text
lunarrender_opengl_*
```

旧的 `lunarrender_gl_*`、`moonmc_*` 和游戏专用上传符号不属于当前边界。

## 3. C stub 和链接

`platform/native/moon.pkg` 编译：

```text
platform.c
file_io.c
image_io.c
```

`backend/opengl/moon.pkg` 编译：

```text
opengl.c
```

CMake 只构建第三方静态库：

```powershell
cmake -S . -B third_party/build -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF
cmake --build third_party/build --config Release
```

MoonBit native 链接需要 GLFW、GLAD 静态库和 Windows 系统库。具体 flags 只写在对应 `moon.pkg` 中，不写入 `renderer`。

## 4. 参数所有权

- 只读的 `FixedArray[Float]` 和 `Bytes` FFI 参数使用 `#borrow`。
- C 代码不得保存 MoonBit 数组、字符串或字节缓冲区的指针到当前调用之外。
- OpenGL VBO 上传完成后，GPU 资源拥有自己的数据；C 不依赖 MoonBit 缓冲区继续存在。
- `Renderer` 的资源键由 MoonBit 传入，C 只在槽位表中保存整数键和 OpenGL 句柄。

## 5. 生命周期和错误阶段

```text
window_create
→ OpenGL context 已存在
→ lunarrender_opengl_init / GLAD
→ Shader / VAO / VBO 创建
→ upload_mesh
→ draw_frame
→ remove_mesh
→ lunarrender_opengl_shutdown
→ window_destroy
```

日志必须区分：

- GLFW 初始化或窗口创建失败。
- OpenGL context 或 GLAD 加载失败。
- Shader 编译或程序链接失败。
- VAO/VBO 创建、数据上传或 OpenGL 错误失败。
- 窗口销毁后继续调用后端。

后端的 `shutdown` 必须先释放 OpenGL 对象，再销毁窗口。重复销毁应安全返回；窗口销毁之后不得再执行 `draw_frame` 或 `upload_mesh`。

## 6. 验证

```powershell
rg "lunarrender_opengl_|lunarrender_platform_" backend platform renderer cmd CMakeLists.txt
moon check --target native
moon test --target native
moon build --target native --release cmd/lunarrender
```

默认构建不需要私有 PMX、VMD、贴图、生成 C 文件或绝对路径。
