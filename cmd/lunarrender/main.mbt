///|
const WindowWidth : Int = 960

///|
const WindowHeight : Int = 540

///|
const KeyEscape : Int = 256

///|
fn identity_matrix() -> FixedArray[Float] {
  FixedArray::makei(16, index => if index % 5 == 0 { 1.0 } else { 0.0 })
}

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
fn mat4_mul(
  left : FixedArray[Float],
  right : FixedArray[Float],
) -> FixedArray[Float] {
  let result : FixedArray[Float] = FixedArray::make(16, 0.0)
  for column = 0; column < 4; column = column + 1 {
    for row = 0; row < 4; row = row + 1 {
      let mut value : Float = 0.0
      for k = 0; k < 4; k = k + 1 {
        value = value + left[k * 4 + row] * right[column * 4 + k]
      }
      result[column * 4 + row] = value
    }
  }
  result
}

///|
fn rotation_x(angle : Double) -> FixedArray[Float] {
  let result = identity_matrix()
  let cosine = Float::from_double(@stdmath.cos(angle))
  let sine = Float::from_double(@stdmath.sin(angle))
  result[5] = cosine
  result[6] = sine
  result[9] = -sine
  result[10] = cosine
  result
}

///|
fn rotation_y(angle : Double) -> FixedArray[Float] {
  let result = identity_matrix()
  let cosine = Float::from_double(@stdmath.cos(angle))
  let sine = Float::from_double(@stdmath.sin(angle))
  result[0] = cosine
  result[2] = -sine
  result[8] = sine
  result[10] = cosine
  result
}

///|
fn translation(x : Float, y : Float, z : Float) -> FixedArray[Float] {
  let result = identity_matrix()
  result[12] = x
  result[13] = y
  result[14] = z
  result
}

///|
fn perspective(aspect : Double) -> FixedArray[Float] {
  let near = 0.1
  let far = 100.0
  let fov = @stdmath.PI * 70.0 / 180.0
  let focal = 1.0 / @stdmath.tan(fov / 2.0)
  let result : FixedArray[Float] = FixedArray::make(16, 0.0)
  result[0] = Float::from_double(focal / aspect)
  result[5] = Float::from_double(focal)
  result[10] = Float::from_double((far + near) / (near - far))
  result[11] = -1.0
  result[14] = Float::from_double(2.0 * far * near / (near - far))
  result
}

///|
fn demo_frame(time : Double, width : Int, height : Int) -> @renderer.FrameData {
  let safe_height = if height > 0 { height } else { 1 }
  let safe_width = if width > 0 { width } else { 1 }
  let aspect = safe_width.to_double() / safe_height.to_double()
  let model = mat4_mul(rotation_y(time * 0.7), rotation_x(time * 0.35))
  let view = translation(0.0, 0.0, -3.2)
  let view_model = mat4_mul(view, model)
  let view_projection = mat4_mul(perspective(aspect), view_model)
  @renderer.FrameData::{
    view_projection,
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
  println("LunarRender: textured cube example started")
  let self_test = self_test_mode()
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
