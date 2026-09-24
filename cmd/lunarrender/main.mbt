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
fn demo_mesh() -> @renderer.MeshData {
  let vertices : Array[Float] = [
    -0.72, -0.58, 0.0, 0.0, 0.0, 0.82,
    0.72, -0.58, 0.0, 1.0, 0.0, 0.82,
    0.0, 0.72, 0.0, 0.5, 1.0, 1.0,
  ]
  @renderer.MeshData::{
    vertices: FixedArray::makei(vertices.length(), index => vertices[index]),
    vertex_count: 3,
    floats_per_vertex: 6,
    material: None,
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
  let mesh = demo_mesh()
  if !renderer.upload_mesh(1, mesh) {
    println("LunarRender: demo mesh upload failed")
    renderer.shutdown()
    @native.window_destroy()
    return
  }
  println("LunarRender: static renderer example started")
  let frame = @renderer.FrameData::{
    view_projection: identity_matrix(),
    viewport_width: WindowWidth,
    viewport_height: WindowHeight,
  }
  let self_test = self_test_mode()
  let start_time = @native.window_time_seconds()
  while !@native.window_should_close() && !@native.window_key_down(KeyEscape) &&
    (!self_test || @native.window_time_seconds() - start_time < 1.0) {
    @native.window_poll_events()
    renderer.draw_frame(frame)
    @native.window_swap_buffers()
  }
  renderer.remove_mesh(1)
  renderer.shutdown()
  @native.window_destroy()
}
