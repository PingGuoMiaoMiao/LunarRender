///|
const WindowWidth : Int = 960

///|
const WindowHeight : Int = 540

///|
const KeyEscape : Int = 256

///|
fn checkerboard_texture() -> @renderer.TextureData {
  let pixels : Array[Byte] = []
  let alpha : Int = 255
  for y = 0; y < 8; y = y + 1 {
    for x = 0; x < 8; x = x + 1 {
      let bright = (x / 2 + y / 2) % 2 == 0
      let value = if bright { 235 } else { 45 }
      pixels.push(value.to_byte())
      pixels.push(value.to_byte())
      pixels.push((if bright { 245 } else { 55 }).to_byte())
      pixels.push(alpha.to_byte())
    }
  }
  @renderer.TextureData::{
    width: 8,
    height: 8,
    rgba: Bytes::from_array(pixels),
  }
}

///|
fn push_vertex(
  vertices : Array[Float],
  x : Float,
  y : Float,
  z : Float,
  u : Float,
  v : Float,
  shade : Float,
) -> Unit {
  vertices.push(x)
  vertices.push(y)
  vertices.push(z)
  vertices.push(u)
  vertices.push(v)
  vertices.push(shade)
}

///|
fn push_face(
  vertices : Array[Float],
  x0 : Float,
  y0 : Float,
  z0 : Float,
  x1 : Float,
  y1 : Float,
  z1 : Float,
  x2 : Float,
  y2 : Float,
  z2 : Float,
  x3 : Float,
  y3 : Float,
  z3 : Float,
  shade : Float,
) -> Unit {
  push_vertex(vertices, x0, y0, z0, 0.0, 0.0, shade)
  push_vertex(vertices, x1, y1, z1, 1.0, 0.0, shade)
  push_vertex(vertices, x2, y2, z2, 1.0, 1.0, shade)
  push_vertex(vertices, x0, y0, z0, 0.0, 0.0, shade)
  push_vertex(vertices, x2, y2, z2, 1.0, 1.0, shade)
  push_vertex(vertices, x3, y3, z3, 0.0, 1.0, shade)
}

///|
fn demo_mesh() -> @renderer.MeshData {
  let vertices : Array[Float] = []
  push_face(vertices, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0)
  push_face(vertices, 1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 0.75)
  push_face(vertices, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0, 0.85)
  push_face(vertices, 1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 0.85)
  push_face(vertices, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0)
  push_face(vertices, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, -1.0, -1.0, 0.6)
  @renderer.MeshData::{
    vertices: FixedArray::makei(vertices.length(), index => vertices[index]),
    vertex_count: 36,
    floats_per_vertex: 6,
    material: Some(@renderer.MaterialHandle::{ id: 1 }),
  }
}

///|
/// Column-major 4x4 multiply. The row loop is unrolled so that each column of
/// `right` is read once: the result is one allocation plus 64 multiply-adds
/// with no inner-loop index arithmetic or repeated bounds checks.
fn mat4_mul(
  left : FixedArray[Float],
  right : FixedArray[Float],
) -> FixedArray[Float] {
  let result : FixedArray[Float] = FixedArray::make(16, 0.0)
  for column = 0; column < 4; column = column + 1 {
    let base = column * 4
    let r0 = right[base]
    let r1 = right[base + 1]
    let r2 = right[base + 2]
    let r3 = right[base + 3]
    result[base] = left[0] * r0 + left[4] * r1 + left[8] * r2 + left[12] * r3
    result[base + 1] = left[1] * r0 +
      left[5] * r1 +
      left[9] * r2 +
      left[13] * r3
    result[base + 2] = left[2] * r0 +
      left[6] * r1 +
      left[10] * r2 +
      left[14] * r3
    result[base + 3] = left[3] * r0 +
      left[7] * r1 +
      left[11] * r2 +
      left[15] * r3
  }
  result
}

///|
/// Reused across frames: the projection only depends on the viewport aspect
/// ratio, and the model-view matrix is composed in place instead of being
/// built from four temporary matrices every frame.
let perspective_cache : FixedArray[Float] = FixedArray::make(16, 0.0)

///|
let perspective_aspect : FixedArray[Double] = FixedArray::make(1, -1.0)

///|
let model_view_scratch : FixedArray[Float] = FixedArray::make(16, 0.0)

///|
fn cached_perspective(aspect : Double) -> FixedArray[Float] {
  if perspective_aspect[0] != aspect {
    perspective_aspect[0] = aspect
    let near = 0.1
    let far = 100.0
    let fov = @stdmath.PI * 70.0 / 180.0
    let focal = 1.0 / @stdmath.tan(fov / 2.0)
    let matrix = perspective_cache
    for index = 0; index < 16; index = index + 1 {
      matrix[index] = 0.0
    }
    matrix[0] = Float::from_double(focal / aspect)
    matrix[5] = Float::from_double(focal)
    matrix[10] = Float::from_double((far + near) / (near - far))
    matrix[11] = -1.0
    matrix[14] = Float::from_double(2.0 * far * near / (near - far))
  }
  perspective_cache
}

///|
fn demo_frame(time : Double, width : Int, height : Int) -> @renderer.FrameData {
  let safe_height = if height > 0 { height } else { 1 }
  let safe_width = if width > 0 { width } else { 1 }
  let aspect = safe_width.to_double() / safe_height.to_double()
  let angle_y = time * 0.7
  let angle_x = time * 0.35
  let cosine_y = @stdmath.cos(angle_y)
  let sine_y = @stdmath.sin(angle_y)
  let cosine_x = @stdmath.cos(angle_x)
  let sine_x = @stdmath.sin(angle_x)
  // view_projection = perspective * translate(0, 0, -3.2) * Ry * Rx.
  // The rotations and the translation are composed directly here (see the
  // "demo_frame matches the matrix helpers" test for the cross-check), so a
  // frame only allocates the result and performs one 4x4 multiply.
  let matrix = model_view_scratch
  matrix[0] = Float::from_double(cosine_y)
  matrix[1] = 0.0
  matrix[2] = Float::from_double(-sine_y)
  matrix[3] = 0.0
  matrix[4] = Float::from_double(sine_y * sine_x)
  matrix[5] = Float::from_double(cosine_x)
  matrix[6] = Float::from_double(cosine_y * sine_x)
  matrix[7] = 0.0
  matrix[8] = Float::from_double(sine_y * cosine_x)
  matrix[9] = Float::from_double(-sine_x)
  matrix[10] = Float::from_double(cosine_y * cosine_x)
  matrix[11] = 0.0
  matrix[12] = 0.0
  matrix[13] = 0.0
  matrix[14] = -3.2
  matrix[15] = 1.0
  @renderer.FrameData::{
    view_projection: mat4_mul(cached_perspective(aspect), matrix),
    viewport_width: width,
    viewport_height: height,
  }
}

///|
fn self_test_mode() -> Bool {
  let args = @env.args()
  for argument in args {
    if argument == "--self-test" {
      return true
    }
  }
  false
}

///|
fn shutdown_after_upload_failure(
  renderer : @opengl.Renderer,
  texture_uploaded : Bool,
  material_uploaded : Bool,
  texture_handle : @renderer.TextureHandle,
  material_handle : @renderer.MaterialHandle,
) -> Unit {
  if material_uploaded {
    renderer.remove_material(material_handle)
  }
  if texture_uploaded {
    renderer.remove_texture(texture_handle)
  }
  renderer.shutdown()
  @native.window_destroy()
}

///|
fn main {
  if !@native.window_create(WindowWidth, WindowHeight) {
    println("LunarRender: platform window creation failed")
    return
  }
  let renderer = match @opengl.Renderer::new() {
    Some(value) => value
    None => {
      println("LunarRender: OpenGL backend initialization failed")
      @native.window_destroy()
      return
    }
  }
  let texture_handle = @renderer.TextureHandle::{ id: 1 }
  let material_handle = @renderer.MaterialHandle::{ id: 1 }
  let mesh_handle = @renderer.MeshHandle::{ id: 1 }
  let texture_uploaded = renderer.upload_texture(texture_handle, checkerboard_texture())
  if !texture_uploaded {
    println("LunarRender: demo texture upload failed")
    shutdown_after_upload_failure(renderer, false, false, texture_handle, material_handle)
    return
  }
  let material = @renderer.MaterialData::{
    texture: Some(texture_handle),
    tint: [1.0, 1.0, 1.0, 1.0],
    alpha_mode: @renderer.AlphaMode::Opaque,
  }
  let material_uploaded = renderer.upload_material(material_handle, material)
  if !material_uploaded {
    println("LunarRender: demo material upload failed")
    shutdown_after_upload_failure(renderer, true, false, texture_handle, material_handle)
    return
  }
  if !renderer.upload_mesh(mesh_handle, demo_mesh()) {
    println("LunarRender: demo mesh upload failed")
    shutdown_after_upload_failure(renderer, true, true, texture_handle, material_handle)
    return
  }
  let self_test = self_test_mode()
  if self_test {
    // Uploading an existing handle again takes the in-place buffer reuse path
    // (no new VAO/VBO), so the smoke test covers it as well.
    if !renderer.upload_mesh(mesh_handle, demo_mesh()) {
      println("LunarRender: demo mesh re-upload failed")
      shutdown_after_upload_failure(renderer, true, true, texture_handle, material_handle)
      return
    }
    println("LunarRender: mesh re-upload reuse ok")
  }
  println("LunarRender: textured cube example started")
  let start_time = @native.window_time_seconds()
  while !@native.window_should_close() && !@native.window_key_down(KeyEscape) &&
    (!self_test || @native.window_time_seconds() - start_time < 1.0) {
    @native.window_poll_events()
    let width = @native.window_framebuffer_width()
    let height = @native.window_framebuffer_height()
    if width > 0 && height > 0 {
      let frame = demo_frame(@native.window_time_seconds(), width, height)
      renderer.draw_frame(frame)
      @native.window_swap_buffers()
    }
  }
  renderer.remove_mesh(mesh_handle)
  renderer.remove_material(material_handle)
  renderer.remove_texture(texture_handle)
  renderer.shutdown()
  @native.window_destroy()
}
