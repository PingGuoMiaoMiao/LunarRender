# LunarRender 渲染基础扩展实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在现有 LunarRender OpenGL 三角形基础上完成可调整窗口、类型化资源、纹理/材质和旋转纹理立方体示例，并保持渲染核心与 MoonMC 游戏逻辑解耦。

**Architecture:** `renderer` 只定义后端无关的数据模型和校验；`backend/opengl` 将类型化资源句柄转换为有限槽位的 OpenGL 对象；`platform/native` 只提供窗口生命周期、输入、时间和 framebuffer 尺寸；`cmd/lunarrender` 生成演示数据并按逆序释放资源。资源引用在 C 层验证，避免材质或网格指向尚未上传的资源。

**Tech Stack:** MoonBit native、C FFI、GLFW 3.4、GLAD 2.0.8、OpenGL 3.3 Core、MSVC、CMake、MoonBit white-box tests。

**Spec:** `docs/superpowers/specs/2026-09-24-render-foundation-design.md`

## Global Constraints

- 目标平台固定为 Windows native、MSVC、GLFW 3.4、GLAD 2.0.8、OpenGL 3.3 Core。
- 本轮不加入 MoonMC 游戏层、无限区块、角色、物理、联网、骨骼动画、MMD GPU Skinning、Vulkan、WebGPU、软件渲染、Shader 热重载或编辑器。
- `renderer` 不包含 C FFI；OpenGL 和窗口实现只存在于 `backend/opengl` 与 `platform/native`。
- 类型化句柄只表达资源命名空间，`id` 必须为非负值，不能直接暴露 OpenGL 对象。
- 纹理输入固定为紧密排列的 RGBA8 字节，字节数严格等于 `width * height * 4`。
- 第一版顶点布局固定为每顶点 6 个 Float：位置 3、UV 2、明暗 1。
- OpenGL 资源槽位上限固定为 256 个网格、256 个纹理和 256 个材质；容量不足必须返回 `false`。
- 私有模型、私有纹理、私有 VMD/PMX 和生成文件不进入默认构建产物或 Git 提交。
- 只有在测试、native 构建、运行自测和 `git diff --check` 均通过后，才允许合并到 `main` 并推送。

## Review Focus

- 负句柄和不同资源类型混用：`valid()` 拒绝负 ID，MoonBit 方法签名不能把 `TextureHandle` 传给 `MeshHandle`。
- RGBA 字节长度错误或零尺寸纹理：纯 MoonBit 校验拒绝，C 层不接收无效上传。
- 材质引用未上传纹理、网格引用未上传材质：C 层返回 `false`，不创建半初始化对象。
- 窗口最小化导致 framebuffer 为 0：示例跳过当前绘制，恢复后使用新的正尺寸调用 `glViewport`。
- 相同句柄重复上传、重复移除和重复 shutdown：旧 GPU 对象先释放并替换，移除和 shutdown 必须幂等。

## File Map

- Modify `renderer/render_types.mbt`: 增加句柄、纹理、材质、透明模式和 viewport 数据模型，并保留现有 6-float 网格校验。
- Modify `renderer/render_types_wbtest.mbt`: 覆盖句柄、纹理、材质、网格材质引用和 framebuffer 校验。
- Modify `backend/opengl/opengl.mbt`: 声明新的 C FFI、管理三类句柄列表、执行 MoonBit 侧输入校验和资源计数。
- Modify `backend/opengl/opengl_wbtest.mbt`: 更新构造数据，覆盖后端状态模型和新数据接口。
- Modify `backend/opengl/opengl.c`: 实现 texture/material/mesh 槽位、shader 采样、alpha blend、viewport 和资源销毁。
- Modify `platform/native/platform.mbt`: 暴露 framebuffer 宽高。
- Modify `platform/native/platform.c`: 使用 `glfwGetFramebufferSize` 读取当前 framebuffer。
- Modify `cmd/lunarrender/main.mbt`: 生成棋盘纹理、材质、36 顶点立方体和旋转视图；处理尺寸变化和逆序销毁。
- Modify `cmd/lunarrender/main_wbtest.mbt`: 测试 checkerboard 字节数、立方体顶点数、矩阵和 self-test 相关纯函数。
- Modify `README.md`: 修正 native 运行路径并说明本轮渲染示例。
- Modify `ARCHITECTURE.md`: 记录资源模型、framebuffer 数据流和后端边界。
- Modify `FFI_BINDINGS.md`: 记录纹理所有权、材质引用、viewport、blend 和销毁规则。
- Modify `ROADMAP.md`: 将本轮渲染基础扩展标记为可验收阶段。
- Modify `PROJECT_PROPOSAL.md`: 增加可验证的渲染基础技术贡献与演示指标。

---

### Task 1: 固定后端无关的资源数据模型

**Files:**
- Modify: `renderer/render_types.mbt`
- Modify: `renderer/render_types_wbtest.mbt`

**Interfaces:**
- Produces `MeshHandle::{ id : Int }`, `TextureHandle::{ id : Int }`, `MaterialHandle::{ id : Int }`。
- Produces `TextureData::{ width : Int, height : Int, rgba : Bytes }`。
- Produces `AlphaMode::{ Opaque; Blend }`。
- Produces `MaterialData::{ texture : TextureHandle?, tint : FixedArray[Float], alpha_mode : AlphaMode }`。
- Extends `MeshData` with `material : MaterialHandle?`。
- Extends `FrameData` with `viewport_width : Int` and `viewport_height : Int`。
- Produces `valid()` methods for all four data families。

- [ ] **Step 1: Write failing tests for handle and resource validation**

Add tests with these exact assertions:

```moonbit
test "resource handles reject negative ids" {
  assert_false(MeshHandle::{ id: -1 }.valid())
  assert_false(TextureHandle::{ id: -1 }.valid())
  assert_false(MaterialHandle::{ id: -1 }.valid())
  assert_true(MeshHandle::{ id: 0 }.valid())
}

test "texture data requires exact rgba8 length" {
  let valid = TextureData::{ width: 2, height: 1, rgba: Bytes::from_array([255, 0, 0, 255, 0, 255, 0, 255]) }
  let short = TextureData::{ width: 2, height: 1, rgba: Bytes::from_array([255, 0, 0, 255]) }
  let zero = TextureData::{ width: 0, height: 1, rgba: Bytes::from_array([]) }
  assert_true(valid.valid())
  assert_false(short.valid())
  assert_false(zero.valid())
}

test "material validates tint and optional texture handle" {
  let valid = MaterialData::{ texture: Some(TextureHandle::{ id: 4 }), tint: [1.0, 1.0, 1.0, 1.0], alpha_mode: Opaque }
  let bad_tint = MaterialData::{ texture: None, tint: [1.0, 1.0, 1.0], alpha_mode: Opaque }
  let bad_texture = MaterialData::{ texture: Some(TextureHandle::{ id: -1 }), tint: [1.0, 1.0, 1.0, 1.0], alpha_mode: Blend }
  assert_true(valid.valid())
  assert_false(bad_tint.valid())
  assert_false(bad_texture.valid())
}

test "mesh and frame validate material and viewport" {
  let mesh = MeshData::{ vertices: FixedArray::make(6, 0.0), vertex_count: 1, floats_per_vertex: 6, material: Some(MaterialHandle::{ id: 2 }) }
  let bad_mesh = MeshData::{ vertices: FixedArray::make(6, 0.0), vertex_count: 1, floats_per_vertex: 6, material: Some(MaterialHandle::{ id: -1 }) }
  let frame = FrameData::{ view_projection: FixedArray::make(16, 0.0), viewport_width: 960, viewport_height: 540 }
  let bad_frame = FrameData::{ view_projection: FixedArray::make(16, 0.0), viewport_width: 0, viewport_height: 540 }
  assert_true(mesh.valid())
  assert_false(bad_mesh.valid())
  assert_true(frame.valid())
  assert_false(bad_frame.valid())
}
```

- [ ] **Step 2: Run the renderer tests and verify they fail for missing fields or methods**

Run:

```powershell
moon test --target native renderer
```

Expected: FAIL because the current structs have no resource handles, texture/material data, or viewport fields.

- [ ] **Step 3: Implement the minimal data model**

Add `valid()` methods with these exact rules:

```moonbit
pub fn MeshHandle::valid(self : MeshHandle) -> Bool { self.id >= 0 }
pub fn TextureHandle::valid(self : TextureHandle) -> Bool { self.id >= 0 }
pub fn MaterialHandle::valid(self : MaterialHandle) -> Bool { self.id >= 0 }

pub fn TextureData::valid(self : TextureData) -> Bool {
  self.width > 0 && self.height > 0 && self.rgba.length() == self.width * self.height * 4
}

pub fn MaterialData::valid(self : MaterialData) -> Bool {
  self.tint.length() == 4 && match self.texture {
    None => true
    Some(handle) => handle.valid()
  }
}
```

Update `MeshData::valid()` to keep `vertex_count >= 0`, `floats_per_vertex == 6`, exact vertex array length, and validate `material` when it is `Some`. Update `FrameData::valid()` to require a 16-element matrix and positive viewport dimensions.

- [ ] **Step 4: Run the focused renderer tests**

Run:

```powershell
moon test --target native renderer
```

Expected: PASS for the new validation tests and the existing mesh/frame tests after their constructors include `material`, `viewport_width`, and `viewport_height`.

- [ ] **Step 5: Commit the data-model boundary**

```powershell
git add renderer/render_types.mbt renderer/render_types_wbtest.mbt
git commit -m "feat: add typed render resources"
```

### Task 2: Add framebuffer-size access to the native platform

**Files:**
- Modify: `platform/native/platform.mbt`
- Modify: `platform/native/platform.c`

**Interfaces:**
- Produces `window_framebuffer_width() -> Int`.
- Produces `window_framebuffer_height() -> Int`.
- Both functions call `glfwGetFramebufferSize` on the existing window and return the current pixel dimensions.

- [ ] **Step 1: Add the MoonBit declarations and run the native check**

Add:

```moonbit
pub extern "C" fn window_framebuffer_width() -> Int =
  "lunarrender_platform_window_framebuffer_width"

pub extern "C" fn window_framebuffer_height() -> Int =
  "lunarrender_platform_window_framebuffer_height"
```

Run:

```powershell
moon check --target native
```

Expected: FAIL at native linking because the C symbols do not exist yet.

- [ ] **Step 2: Implement the exact C symbols**

In `platform/native/platform.c`, implement both functions with the existing static GLFW window pointer and this behavior:

```c
int32_t lunarrender_platform_window_framebuffer_width(void) {
    int width = 0;
    int height = 0;
    if (g_window != NULL) {
        glfwGetFramebufferSize(g_window, &width, &height);
    }
    return (int32_t)width;
}
```

The height function uses the same call and returns `height`. If no window exists, both return `0`. Do not invent a second window state or cache dimensions in MoonBit.

- [ ] **Step 3: Verify the platform symbols through the native check**

Run:

```powershell
moon check --target native
```

Expected: PASS with the new C functions linked through `platform/native/moon.pkg`.

- [ ] **Step 4: Commit the platform boundary**

```powershell
git add platform/native/platform.mbt platform/native/platform.c
git commit -m "feat: expose framebuffer dimensions"
```

### Task 3: Implement typed texture, material, and mesh uploads in the OpenGL backend

**Files:**
- Modify: `backend/opengl/opengl.mbt`
- Modify: `backend/opengl/opengl_wbtest.mbt`
- Modify: `backend/opengl/opengl.c`

**Interfaces:**
- Consumes `@renderer.TextureHandle`, `@renderer.MaterialHandle`, `@renderer.MeshHandle`, `@renderer.TextureData`, `@renderer.MaterialData`, `@renderer.MeshData`, and `@renderer.FrameData`.
- Produces `Renderer::upload_texture(handle, data) -> Bool`.
- Produces `Renderer::remove_texture(handle) -> Unit`.
- Produces `Renderer::upload_material(handle, data) -> Bool`.
- Produces `Renderer::remove_material(handle) -> Unit`.
- Changes `Renderer::upload_mesh(handle, mesh) -> Bool` and `remove_mesh(handle) -> Unit` to typed handles.
- Changes `Renderer::draw_frame(frame) -> Unit` to pass both matrix and viewport dimensions.
- Produces `uploaded_texture_count() -> Int`, `uploaded_material_count() -> Int`, and keeps `uploaded_mesh_count() -> Int`.

- [ ] **Step 1: Extend the MoonBit FFI declarations before implementing behavior**

Add these declarations in `backend/opengl/opengl.mbt`:

```moonbit
#borrow(rgba)
extern "C" fn opengl_upload_texture(key : Int, width : Int, height : Int, rgba : Bytes) -> Bool =
  "lunarrender_opengl_upload_texture"

extern "C" fn opengl_remove_texture(key : Int) -> Unit =
  "lunarrender_opengl_remove_texture"

#borrow(tint)
extern "C" fn opengl_upload_material(key : Int, texture_key : Int, tint : FixedArray[Float], alpha_mode : Int) -> Bool =
  "lunarrender_opengl_upload_material"

extern "C" fn opengl_remove_material(key : Int) -> Unit =
  "lunarrender_opengl_remove_material"

#borrow(vertices)
extern "C" fn opengl_upload_mesh(key : Int, material_key : Int, vertices : FixedArray[Float], vertex_count : Int, floats_per_vertex : Int) -> Bool =
  "lunarrender_opengl_upload_mesh"

#borrow(view_projection)
extern "C" fn opengl_draw_frame(view_projection : FixedArray[Float], viewport_width : Int, viewport_height : Int) -> Unit =
  "lunarrender_opengl_draw_frame"
```

Run:

```powershell
moon check --target native
```

Expected: FAIL only because the C implementation has not yet supplied the new symbol signatures.

- [ ] **Step 2: Add focused MoonBit state tests before the C implementation**

Update the backend white-box tests so that they construct `MeshData` with `material: None` or `Some(MaterialHandle::{ id: 3 })`, and `FrameData` with `viewport_width: 960` and `viewport_height: 540`. Add:

```moonbit
test "renderer state tracks independent resource namespaces" {
  let renderer = Renderer::{ initialized: false, uploaded_mesh_keys: [], uploaded_texture_keys: [], uploaded_material_keys: [] }
  assert_eq(renderer.uploaded_mesh_count(), 0)
  assert_eq(renderer.uploaded_texture_count(), 0)
  assert_eq(renderer.uploaded_material_count(), 0)
}
```

- [ ] **Step 3: Implement MoonBit-side validation and tracking**

Replace the untyped integer list with three arrays of `Int`. Before each native call:

```moonbit
if !self.initialized || !handle.valid() || !texture.valid() { return false }
```

For materials, map `None` texture to `-1`, `Some(TextureHandle::{ id })` to `id`, map `Opaque` to `0` and `Blend` to `1`. For meshes, map `None` material to `-1` and `Some(MaterialHandle::{ id })` to `id`. On successful replacement, retain one occurrence of the key; on removal, filter the corresponding list. On shutdown, clear all three lists after calling the native shutdown function.

- [ ] **Step 4: Implement finite C resource slots and ownership**

In `backend/opengl/opengl.c`, add three slot arrays with capacity 256:

```c
typedef struct {
    int active;
    int key;
    GLuint texture;
} lunarrender_texture_slot;

typedef struct {
    int active;
    int key;
    int texture_key;
    float tint[4];
    int alpha_mode;
} lunarrender_material_slot;
```

Extend the mesh slot with `material_key`. Implement these exact rules:

1. `lunarrender_opengl_upload_texture` rejects nonpositive dimensions or `rgba == NULL`, replaces an existing key only after deleting its old texture, calls `glGenTextures`, `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba)`, sets `GL_NEAREST` and `GL_CLAMP_TO_EDGE`, and returns `false` on a GL error.
2. `lunarrender_opengl_upload_material` rejects a nonzero alpha mode other than `1`, rejects a missing tint pointer, and rejects a nonnegative `texture_key` unless an active texture slot exists. Reupload replaces the stored material data.
3. `lunarrender_opengl_upload_mesh` keeps the existing 6-float VAO/VBO layout, rejects a nonnegative `material_key` unless an active material slot exists, and replaces an existing mesh key after deleting its VAO/VBO.
4. Remove functions are safe for missing keys and only delete active OpenGL objects.
5. `lunarrender_opengl_shutdown` deletes every active texture, mesh VAO/VBO, and shader program, clears active flags, and is safe if called twice.

Log every successful upload with the resource type and key, and log rejected references with the exact referenced key.

- [ ] **Step 5: Extend shader and draw state**

Change the fragment shader to accept:

```glsl
uniform sampler2D u_texture;
uniform int u_has_texture;
uniform vec4 u_tint;
```

The fragment color is `texture(u_texture, v_uv) * u_tint` when `u_has_texture == 1`, otherwise `vec4(v_shade, v_shade, v_shade, 1.0) * u_tint`. Discard fragments with alpha below `0.03`. In `draw_frame`, call `glViewport(0, 0, viewport_width, viewport_height)` only when both values are positive, clear color/depth, look up each mesh material, bind its texture when present, set tint, enable blending only for `Blend`, and call `glDrawArrays`.

- [ ] **Step 6: Run backend tests and native linking**

Run:

```powershell
moon test --target native backend/opengl
moon check --target native
```

Expected: PASS for pure backend tests and symbol resolution after the C implementation is complete.

- [ ] **Step 7: Commit the OpenGL resource boundary**

```powershell
git add backend/opengl/opengl.mbt backend/opengl/opengl_wbtest.mbt backend/opengl/opengl.c
git commit -m "feat: add OpenGL texture and material resources"
```

### Task 4: Replace the static triangle with a textured rotating cube

**Files:**
- Modify: `cmd/lunarrender/main.mbt`
- Modify: `cmd/lunarrender/main_wbtest.mbt`
- Modify: `cmd/lunarrender/moon.pkg`

**Interfaces:**
- Consumes the typed renderer data and platform framebuffer functions from Tasks 1–3.
- Produces `checkerboard_texture() -> @renderer.TextureData`.
- Produces `demo_mesh() -> @renderer.MeshData` with 36 vertices and stride 6.
- Produces `demo_frame(time : Double, width : Int, height : Int) -> @renderer.FrameData`.

- [ ] **Step 1: Write failing demo tests**

Add these exact tests:

```moonbit
test "checkerboard texture has exact rgba8 size" {
  let texture = checkerboard_texture()
  assert_eq(texture.width, 8)
  assert_eq(texture.height, 8)
  assert_eq(texture.rgba.length(), 8 * 8 * 4)
  assert_true(texture.valid())
}

test "demo mesh is a six-face cube" {
  let mesh = demo_mesh()
  assert_eq(mesh.vertex_count, 36)
  assert_eq(mesh.floats_per_vertex, 6)
  assert_eq(mesh.vertices.length(), 36 * 6)
  assert_true(mesh.valid())
}

test "demo frame follows supplied viewport" {
  let frame = demo_frame(0.0, 1280, 720)
  assert_eq(frame.viewport_width, 1280)
  assert_eq(frame.viewport_height, 720)
  assert_eq(frame.view_projection.length(), 16)
  assert_true(frame.valid())
}
```

- [ ] **Step 2: Run the command-package tests to verify they fail**

Run:

```powershell
moon test --target native cmd/lunarrender
```

Expected: FAIL because the current command package creates a triangle, has no checkerboard function, and `FrameData` has no viewport fields.

- [ ] **Step 3: Implement checkerboard and cube generation in MoonBit**

Generate an 8×8 RGBA texture using `Bytes::from_array`; each 2×2 cell alternates between `[235, 235, 235, 255]` and `[45, 45, 55, 255]`. Generate six faces, two triangles per face, with 3D positions in `[-1.0, 1.0]`, UVs `(0,0)`, `(1,0)`, `(1,1)`, `(0,1)`, and shade values `1.0`, `0.85`, `0.75`, or `0.6`. Store `material: Some(MaterialHandle::{ id: 1 })`.

Use a local `push_vertex(vertices, x, y, z, u, v, shade)` helper that appends exactly six Float values. Use a local `push_face` helper that appends the six vertices for a face with the two triangles `(0,1,2)` and `(0,2,3)`.

- [ ] **Step 4: Implement the demo matrix and frame construction**

Add this exact import to `cmd/lunarrender/moon.pkg`, using the repository’s existing alias:

```text
"moonbitlang/core/math" @stdmath,
```

Use `@stdmath.sin`, `@stdmath.cos`, and `@stdmath.tan`. Implement column-major 4×4 multiplication and construct:

```text
model = rotation_y(time * 0.7) * rotation_x(time * 0.35)
view  = translation(0, 0, -3.2)
proj  = perspective(70 degrees, width / height, 0.1, 100.0)
view_projection = proj * view * model
```

`demo_frame` must return a 16-element matrix and the supplied positive viewport. The main loop reads framebuffer width and height after event polling; if either is nonpositive, it skips `draw_frame` and swaps no GPU frame until the next loop iteration. Otherwise it draws with the current dimensions.

- [ ] **Step 5: Wire upload, loop, and reverse teardown**

Use these typed handles in `main`:

```moonbit
let texture_handle = @renderer.TextureHandle::{ id: 1 }
let material_handle = @renderer.MaterialHandle::{ id: 1 }
let mesh_handle = @renderer.MeshHandle::{ id: 1 }
```

Upload in order: texture, material, mesh. If an upload fails, remove previously uploaded resources, shut down the renderer, destroy the window, and return. Each frame calls `demo_frame(current_time, framebuffer_width, framebuffer_height)`. On exit remove mesh, remove material, remove texture, call `renderer.shutdown()`, then `@native.window_destroy()`.

- [ ] **Step 6: Run command tests and build the demo**

Run:

```powershell
moon test --target native cmd/lunarrender
moon check --target native
```

Expected: PASS with a 36-vertex mesh and valid rotating frame data.

- [ ] **Step 7: Commit the runnable cube example**

```powershell
git add cmd/lunarrender/main.mbt cmd/lunarrender/main_wbtest.mbt cmd/lunarrender/moon.pkg
git commit -m "feat: add textured rotating cube demo"
```

### Task 5: Align documentation, run full verification, and prepare delivery

**Files:**
- Modify: `README.md`
- Modify: `ARCHITECTURE.md`
- Modify: `FFI_BINDINGS.md`
- Modify: `ROADMAP.md`
- Modify: `PROJECT_PROPOSAL.md`

**Interfaces:**
- Documentation must describe the implemented typed resource API, framebuffer behavior, RGBA ownership, shader inputs, alpha modes, and reverse teardown order.
- Documentation must keep the project boundary as a generic rendering runtime; MoonMC game logic remains a separate project.

- [ ] **Step 1: Update documentation from the implementation**

Make these exact corrections:

1. In `README.md`, use `.\scripts\resolve-runtime.ps1` before launching the executable so both standalone and temporary-workspace MoonBit builds work; describe the checkerboard cube and state that resizing uses framebuffer pixels.
2. In `ARCHITECTURE.md`, add the flow `MoonBit data -> typed handle -> OpenGL slot -> draw_frame`, and state that texture/material/mesh ownership belongs to the OpenGL backend.
3. In `FFI_BINDINGS.md`, document `#borrow` for bytes/fixed arrays, that C copies texture bytes into a GL texture during upload, that remove/shutdown release native objects, and that `glViewport` uses the current framebuffer dimensions.
4. In `ROADMAP.md`, mark the four delivered items as the “渲染基础扩展” milestone and leave game, MoonMC integration, and MMD skinning as later work.
5. In `PROJECT_PROPOSAL.md`, describe the reusable contribution as a backend-neutral MoonBit render data boundary with a native OpenGL reference backend, typed resources, and a runnable validation scene.

- [ ] **Step 2: Run MoonBit and third-party builds**

Run from the worktree root:

```powershell
moon check --target native
moon test --target native
cmake --build third_party/build --config Release
moon build --target native --release cmd/lunarrender
```

Expected: all commands exit with code 0.

- [ ] **Step 3: Run the native self-test and inspect logs**

Run:

```powershell
$exe = .\scripts\resolve-runtime.ps1
& $exe --self-test
```

Expected logs include: window creation, OpenGL version, texture upload, material upload, mesh upload, OpenGL backend destruction, and platform window destruction. The process must exit with code 0 after approximately one second.

- [ ] **Step 4: Check symbol names and whitespace**

Run:

```powershell
rg "moonmc_|lunarrender_" backend platform cmd CMakeLists.txt
git diff --check
git status --short
```

Expected: all native symbols use the `lunarrender_` prefix, no whitespace errors are reported, and only the intended implementation/documentation files are changed.

- [ ] **Step 5: Commit the complete feature**

```powershell
git add README.md ARCHITECTURE.md FFI_BINDINGS.md ROADMAP.md PROJECT_PROPOSAL.md
git commit -m "docs: document render foundation extension"
```

- [ ] **Step 6: Integrate after review**

After the worktree passes all checks and receives review, fast-forward the main branch and push:

```powershell
git -C 'C:\Users\chen\Documents\ChatGPT\9月moonbit\LunarRender' merge --ff-only codex/render-foundation
git -C 'C:\Users\chen\Documents\ChatGPT\9月moonbit\LunarRender' push origin main
```

Do not rewrite history, delete the existing MIT commit, or copy private model assets into the repository.
