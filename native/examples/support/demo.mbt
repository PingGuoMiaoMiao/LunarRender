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
fn frame(time : Double, width : Int, height : Int) -> @renderer.FrameData {
  let safe_height = if height > 0 { height } else { 1 }
  let safe_width = if width > 0 { width } else { 1 }
  let aspect = safe_width.to_double() / safe_height.to_double()
  let model = rotation_y(time * 0.7)
  let view = translation(0.0, 0.0, -3.2)
  let view_model = mat4_mul(view, model)
  @renderer.FrameData::{
    view_projection: mat4_mul(perspective(aspect), view_model),
    viewport_width: width,
    viewport_height: height,
  }
}

///|
fn self_test_mode() -> Bool {
  for argument in @env.args() {
    if argument == "--self-test" {
      return true
    }
  }
  false
}

///|
/// Run a native demo with backend-neutral mesh data.
pub fn run(
  title : String,
  handles : Array[@renderer.MeshHandle],
  meshes : Array[@renderer.MeshData],
) -> Unit {
  if handles.length() != meshes.length() {
    println("LunarRenderNative: example input length mismatch")
    return
  }
  if !@native.window_create(WindowWidth, WindowHeight) {
    println("LunarRenderNative: platform window creation failed")
    return
  }
  let renderer = match @opengl.Renderer::new() {
    Some(value) => value
    None => {
      println("LunarRenderNative: OpenGL backend initialization failed")
      @native.window_destroy()
      return
    }
  }
  let mut uploaded = 0
  let mut upload_ok = true
  for index = 0; index < meshes.length(); index = index + 1 {
    if renderer.upload_mesh(handles[index], meshes[index]) {
      uploaded = uploaded + 1
    } else {
      upload_ok = false
    }
  }
  if !upload_ok {
    println("LunarRenderNative: mesh upload failed")
    for index = 0; index < uploaded; index = index + 1 {
      renderer.remove_mesh(handles[index])
    }
    renderer.shutdown()
    @native.window_destroy()
    return
  }
  println(title)
  let self_test = self_test_mode()
  let start_time = @native.window_time_seconds()
  while !@native.window_should_close() &&
        !@native.window_key_down(KeyEscape) &&
        (!self_test || @native.window_time_seconds() - start_time < 1.0) {
    @native.window_poll_events()
    let width = @native.window_framebuffer_width()
    let height = @native.window_framebuffer_height()
    if width > 0 && height > 0 {
      renderer.draw_frame(frame(@native.window_time_seconds(), width, height))
      @native.window_swap_buffers()
    }
  }
  for handle in handles {
    renderer.remove_mesh(handle)
  }
  renderer.shutdown()
  @native.window_destroy()
}
