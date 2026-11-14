///|
fn triangle_mesh() -> @renderer.MeshData {
  @renderer.MeshData::{
    vertices: [
      -1.2, -0.9, 0.0, 0.0, 0.0, 1.0, 1.2, -0.9, 0.0, 1.0, 0.0, 0.7, 0.0, 1.1, 0.0,
      0.5, 1.0, 0.9,
    ],
    vertex_count: 3,
    floats_per_vertex: 6,
    material: None,
  }
}

///|
fn link_native_backend() -> Unit {
  if false {
    let _ = @opengl.Renderer::new()
    let _ = @native.window_should_close()
  }
}

///|
fn main {
  link_native_backend()
  @support.run(
    "LunarRenderNative: triangle example started",
    [@renderer.MeshHandle::{ id: 1, }],
    [triangle_mesh()],
  )
}
