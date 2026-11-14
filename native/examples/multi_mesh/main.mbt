///|
fn mesh(x : Float, shade : Float) -> @renderer.MeshData {
  @renderer.MeshData::{
    vertices: [
      x - 0.7,
      -0.7,
      0.0,
      0.0,
      0.0,
      shade,
      x + 0.7,
      -0.7,
      0.0,
      1.0,
      0.0,
      shade,
      x + 0.7,
      0.7,
      0.0,
      1.0,
      1.0,
      shade,
      x - 0.7,
      -0.7,
      0.0,
      0.0,
      0.0,
      shade,
      x + 0.7,
      0.7,
      0.0,
      1.0,
      1.0,
      shade,
      x - 0.7,
      0.7,
      0.0,
      0.0,
      1.0,
      shade,
    ],
    vertex_count: 6,
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
    "LunarRenderNative: multi_mesh example started",
    [@renderer.MeshHandle::{ id: 1, }, @renderer.MeshHandle::{ id: 2, }],
    [mesh(-0.85, 0.95), mesh(0.85, 0.55)],
  )
}
