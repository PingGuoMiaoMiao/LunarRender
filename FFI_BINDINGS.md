# LunarRender FFI Bindings

## 固定工具链

| 项目 | 固定值 |
|---|---|
| 平台 | Windows native x64 |
| C 工具链 | Visual Studio 2022 MSVC |
| 窗口/输入 | GLFW 3.4 |
| OpenGL loader | GLAD 2.0.8 |
| 图形 API | OpenGL 3.3 Core |
| 构建 | CMake + MoonBit native |

## 符号命名

所有 MoonBit `extern "C"` 声明都映射到 `lunarrender_*` C 符号：

```text
lunarrender_window_create
lunarrender_gl_init
lunarrender_gl_upload_chunk_mesh
lunarrender_gl_upload_player_mesh
lunarrender_file_read
lunarrender_image_decode_rgba
lunarrender_save_write_temp
```

LunarRender 不得继续使用旧项目的 C FFI 前缀。MoonMC 核心没有任何 C FFI。

## MoonBit/C 边界

`FixedArray[Float]` 和 `Bytes` 使用 `#borrow(...)`，C 只在调用期间读取，不转移 MoonBit 所有权。C stub 不读取 MoonBit 结构体内部布局，不向 MoonMC 返回窗口、纹理、VAO、VBO、FILE 或 COM 指针。

## 窗口、OpenGL 和输入

窗口层负责 `window_create`、关闭状态、事件轮询、交换缓冲、时间、键盘、鼠标按钮和鼠标增量。创建时请求 OpenGL 3.3 Core；失败后不得继续初始化 GLAD 或上传资源。

OpenGL 层负责 `gl_init`、shader、纹理、区块/玩家网格上传、区块移除、绘制、HUD、选中框和 `gl_destroy_renderer`。绘制只接受 MoonMC 生成的连续矩阵、顶点数组和标量。

## 文件和图片

`lunarrender_file_read`、`lunarrender_file_exists`、`lunarrender_image_decode_rgba`、`lunarrender_save_write_temp`、`lunarrender_save_replace_temp` 和 `lunarrender_save_ensure_directory` 属于客户端文件边界。WIC 解码结果固定为：4 字节 little-endian width、4 字节 little-endian height、随后是 `width × height × 4` RGBA 字节。

资源包相对路径由 MoonMC manifest 校验逻辑生成。当前 C 文件层检查空指针、空路径、路径长度和文件属性；它不替代 manifest 校验，也不把私有模型路径写入默认构建。私有模型路径只能由本地可忽略准备脚本提供。

## 生命周期与错误处理

```text
GLFW 初始化
→ window/context 创建
→ GLAD 加载
→ shader/VAO/VBO/纹理创建
→ 资源解码和纹理上传
→ 区块/玩家上传和绘制
→ OpenGL 资源销毁
→ GLFW window 销毁
```

必须区分并记录：GLFW 初始化失败、窗口/context 创建失败、GLAD 加载失败、shader 编译/链接失败、纹理上传失败、VAO/VBO 创建失败、文件/WIC/存档失败，以及 window 销毁后的 OpenGL 调用。`gl_destroy_renderer` 必须幂等，并且发生在 `window_destroy` 之前。

## 可选 MMD

MMD 解析、动画和模型纹理不属于默认 FFI。启用时必须使用独立生成 C 文件和独立构建入口；未启用时 `platform/native/moon.pkg` 不列出私有 MMD stub，基础构建不应链接这些符号。
