# LunarRender 渲染基础扩展设计

## 1. 目标

在现有可运行的 OpenGL 静态三角形基础上，完成第一轮通用渲染基础扩展：窗口尺寸变化、类型化资源句柄、纹理/材质上传，以及带纹理和旋转矩阵的立方体示例。

这轮仍然只扩展 LunarRender 渲染运行时，不恢复 MoonMC 依赖，也不加入世界、玩家、Steve、物理、HUD、物品栏、命令或存档。

## 2. 当前边界

### 保留

- Windows native。
- GLFW 3.4 窗口和输入。
- GLAD 2.0.8 与 OpenGL 3.3 Core。
- `renderer` 纯 MoonBit 数据接口。
- `backend/opengl` GPU 资源和绘制实现。
- `formats/mmd` 仅作为独立格式解析模块。

### 本轮不做

- MoonMC 游戏层接入。
- 无限区块、角色、物理或联网。
- 骨骼动画和 MMD GPU Skinning。
- Vulkan、WebGPU 或软件渲染后端。
- Shader 热重载和编辑器。

## 3. 公共数据模型

`renderer` 新增类型化句柄，防止把不同资源的整数键混用：

```moonbit
pub(all) struct MeshHandle { id : Int }
pub(all) struct TextureHandle { id : Int }
pub(all) struct MaterialHandle { id : Int }
```

句柄的 `id` 必须为非负值。句柄只表达资源命名空间，不暴露 OpenGL 对象。

纹理和材质：

```moonbit
pub(all) struct TextureData {
  width : Int
  height : Int
  rgba : Bytes
}

pub(all) enum AlphaMode {
  Opaque
  Blend
}

pub(all) struct MaterialData {
  texture : TextureHandle?
  tint : FixedArray[Float]
  alpha_mode : AlphaMode
}
```

`TextureData` 要求宽、高为正数，RGBA 字节数严格为 `width * height * 4`；材质 tint 严格为 4 个 Float。

网格在原字段基础上增加可选材质：

```moonbit
pub(all) struct MeshData {
  vertices : FixedArray[Float]
  vertex_count : Int
  floats_per_vertex : Int
  material : MaterialHandle?
}
```

第一版顶点布局仍为 6 Float：位置 3、UV 2、明暗 1。`MeshData::valid` 继续校验精确数组长度。

帧数据增加实际 framebuffer 尺寸：

```moonbit
pub(all) struct FrameData {
  view_projection : FixedArray[Float]
  viewport_width : Int
  viewport_height : Int
}
```

## 4. Renderer 后端边界

OpenGL 后端提供：

```moonbit
pub fn Renderer::upload_texture(
  self : Renderer,
  handle : @renderer.TextureHandle,
  texture : @renderer.TextureData,
) -> Bool

pub fn Renderer::remove_texture(
  self : Renderer,
  handle : @renderer.TextureHandle,
) -> Unit

pub fn Renderer::upload_material(
  self : Renderer,
  handle : @renderer.MaterialHandle,
  material : @renderer.MaterialData,
) -> Bool

pub fn Renderer::remove_material(
  self : Renderer,
  handle : @renderer.MaterialHandle,
) -> Unit

pub fn Renderer::upload_mesh(
  self : Renderer,
  handle : @renderer.MeshHandle,
  mesh : @renderer.MeshData,
) -> Bool
```

OpenGL C 层仍然可以使用有限槽位，但 MoonBit 公共接口不再接受无类型的资源键。资源上传失败必须返回 `false` 并输出阶段日志。

## 5. 窗口尺寸和绘制

平台层提供当前 framebuffer 宽高。每帧由示例构造包含实际尺寸的 `FrameData`，OpenGL 后端在 `draw_frame` 中调用 `glViewport`，不再固定 `960×540`。

Shader 支持：

- 无纹理材质使用 tint 颜色。
- 有纹理材质使用 RGBA 纹理并乘以 tint。
- `Opaque` 关闭混合。
- `Blend` 开启 `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`。
- 深度测试继续开启。

## 6. 示例程序

`cmd/lunarrender` 改为绘制带棋盘纹理的立方体：

1. 创建窗口和 OpenGL 后端。
2. 由 MoonBit 生成 2D checkerboard RGBA 字节。
3. 上传 `TextureHandle` 和 `MaterialHandle`。
4. 生成立方体 36 个顶点并上传 `MeshHandle`。
5. 每帧按时间更新旋转矩阵和 viewport。
6. `Esc` 或 `--self-test` 退出。
7. 按网格、材质、纹理、OpenGL 后端、窗口的逆序销毁。

## 7. 测试和验收

纯 MoonBit 测试必须覆盖：

- 三种句柄拒绝负 ID。
- 合法和非法 RGBA 字节长度。
- 材质 tint 长度和句柄校验。
- 网格材质为空和有材质两种情况。
- 帧矩阵和 viewport 尺寸校验。
- 立方体顶点数为 36，顶点步幅为 6。

工程验收：

```powershell
moon check --target native
moon test --target native
cmake --build third_party/build --config Release
moon build --target native --release cmd/lunarrender
.\_build\native\release\build\cmd\lunarrender\lunarrender.exe --self-test
```

运行日志必须能确认窗口尺寸、OpenGL 版本、纹理上传、材质上传、网格上传和反向销毁顺序。
