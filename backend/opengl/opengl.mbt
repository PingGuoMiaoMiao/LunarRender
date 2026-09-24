///|
extern "C" fn opengl_init() -> Bool = "lunarrender_opengl_init"

///|
#borrow(vertices)
extern "C" fn opengl_upload_mesh(
  key : Int,
  vertices : FixedArray[Float],
  vertex_count : Int,
  floats_per_vertex : Int,
) -> Bool = "lunarrender_opengl_upload_mesh"

///|
extern "C" fn opengl_remove_mesh(key : Int) -> Unit =
  "lunarrender_opengl_remove_mesh"

///|
#borrow(view_projection)
extern "C" fn opengl_draw_frame(view_projection : FixedArray[Float]) -> Unit =
  "lunarrender_opengl_draw_frame"

///|
extern "C" fn opengl_shutdown() -> Unit = "lunarrender_opengl_shutdown"

/// A GPU resource manager implemented by one rendering backend.
pub(all) struct Renderer {
  mut initialized : Bool
  mut uploaded_keys : Array[Int]
}

///|
pub fn Renderer::new() -> Renderer? {
  if opengl_init() {
    Some(Renderer::{ initialized: true, uploaded_keys: [] })
  } else {
    None
  }
}

///|
pub fn Renderer::upload_mesh(
  self : Renderer,
  key : Int,
  mesh : @renderer.MeshData,
) -> Bool {
  if !self.initialized || !mesh.valid() {
    return false
  }
  let uploaded = opengl_upload_mesh(
    key,
    mesh.vertices,
    mesh.vertex_count,
    mesh.floats_per_vertex,
  )
  if uploaded {
    let mut exists = false
    for existing_key in self.uploaded_keys {
      if existing_key == key {
        exists = true
      }
    }
    if !exists {
      self.uploaded_keys.push(key)
    }
  }
  uploaded
}

///|
pub fn Renderer::remove_mesh(self : Renderer, key : Int) -> Unit {
  if self.initialized {
    opengl_remove_mesh(key)
  }
  let remaining : Array[Int] = []
  for existing_key in self.uploaded_keys {
    if existing_key != key {
      remaining.push(existing_key)
    }
  }
  self.uploaded_keys = remaining
}

///|
pub fn Renderer::draw_frame(
  self : Renderer,
  frame : @renderer.FrameData,
) -> Unit {
  if self.initialized && frame.valid() {
    opengl_draw_frame(frame.view_projection)
  }
}

///|
pub fn Renderer::uploaded_mesh_count(self : Renderer) -> Int {
  self.uploaded_keys.length()
}

///|
pub fn Renderer::shutdown(self : Renderer) -> Unit {
  if self.initialized {
    opengl_shutdown()
    self.initialized = false
    self.uploaded_keys = []
  }
}
