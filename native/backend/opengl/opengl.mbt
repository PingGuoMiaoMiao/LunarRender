///|
extern "C" fn opengl_init() -> Bool = "lunarrender_opengl_init"

///|
#borrow(rgba)
extern "C" fn opengl_upload_texture(
  key : Int,
  width : Int,
  height : Int,
  rgba : Bytes,
) -> Bool = "lunarrender_opengl_upload_texture"

///|
extern "C" fn opengl_remove_texture(key : Int) -> Unit =
  "lunarrender_opengl_remove_texture"

///|
#borrow(tint)
extern "C" fn opengl_upload_material(
  key : Int,
  texture_key : Int,
  tint : FixedArray[Float],
  alpha_mode : Int,
) -> Bool = "lunarrender_opengl_upload_material"

///|
extern "C" fn opengl_remove_material(key : Int) -> Unit =
  "lunarrender_opengl_remove_material"

///|
#borrow(vertices)
extern "C" fn opengl_upload_mesh(
  key : Int,
  material_key : Int,
  vertices : FixedArray[Float],
  vertex_count : Int,
  floats_per_vertex : Int,
) -> Bool = "lunarrender_opengl_upload_mesh"

///|
extern "C" fn opengl_remove_mesh(key : Int) -> Unit =
  "lunarrender_opengl_remove_mesh"

///|
#borrow(view_projection)
extern "C" fn opengl_draw_frame(
  view_projection : FixedArray[Float],
  viewport_width : Int,
  viewport_height : Int,
) -> Unit = "lunarrender_opengl_draw_frame"

///|
extern "C" fn opengl_shutdown() -> Unit = "lunarrender_opengl_shutdown"

/// A GPU resource manager implemented by one rendering backend.
pub(all) struct Renderer {
  mut initialized : Bool
  mut uploaded_mesh_keys : Array[Int]
  mut uploaded_texture_keys : Array[Int]
  mut uploaded_material_keys : Array[Int]
}

///|
fn append_key(keys : Array[Int], key : Int) -> Unit {
  let mut exists = false
  for existing_key in keys {
    if existing_key == key {
      exists = true
    }
  }
  if !exists {
    keys.push(key)
  }
}

///|
fn remove_key(keys : Array[Int], key : Int) -> Array[Int] {
  let remaining : Array[Int] = []
  for existing_key in keys {
    if existing_key != key {
      remaining.push(existing_key)
    }
  }
  remaining
}

///|
fn alpha_mode_code(mode : @renderer.AlphaMode) -> Int {
  match mode {
    @renderer.AlphaMode::Opaque => 0
    @renderer.AlphaMode::Blend => 1
  }
}

///|
pub fn Renderer::new() -> Renderer? {
  if opengl_init() {
    Some(Renderer::{
      initialized: true,
      uploaded_mesh_keys: [],
      uploaded_texture_keys: [],
      uploaded_material_keys: [],
    })
  } else {
    None
  }
}

///|
pub fn Renderer::upload_texture(
  self : Renderer,
  handle : @renderer.TextureHandle,
  texture : @renderer.TextureData,
) -> Bool {
  if !self.initialized || !handle.valid() || !texture.valid() {
    return false
  }
  let uploaded = opengl_upload_texture(
    handle.id,
    texture.width,
    texture.height,
    texture.rgba,
  )
  if uploaded {
    append_key(self.uploaded_texture_keys, handle.id)
  }
  uploaded
}

///|
pub fn Renderer::remove_texture(
  self : Renderer,
  handle : @renderer.TextureHandle,
) -> Unit {
  if self.initialized && handle.valid() {
    opengl_remove_texture(handle.id)
  }
  if handle.valid() {
    self.uploaded_texture_keys = remove_key(self.uploaded_texture_keys, handle.id)
  }
}

///|
pub fn Renderer::upload_material(
  self : Renderer,
  handle : @renderer.MaterialHandle,
  material : @renderer.MaterialData,
) -> Bool {
  if !self.initialized || !handle.valid() || !material.valid() {
    return false
  }
  let texture_key = match material.texture {
    None => -1
    Some(texture_handle) => texture_handle.id
  }
  let uploaded = opengl_upload_material(
    handle.id,
    texture_key,
    material.tint,
    alpha_mode_code(material.alpha_mode),
  )
  if uploaded {
    append_key(self.uploaded_material_keys, handle.id)
  }
  uploaded
}

///|
pub fn Renderer::remove_material(
  self : Renderer,
  handle : @renderer.MaterialHandle,
) -> Unit {
  if self.initialized && handle.valid() {
    opengl_remove_material(handle.id)
  }
  if handle.valid() {
    self.uploaded_material_keys = remove_key(self.uploaded_material_keys, handle.id)
  }
}

///|
pub fn Renderer::upload_mesh(
  self : Renderer,
  handle : @renderer.MeshHandle,
  mesh : @renderer.MeshData,
) -> Bool {
  if !self.initialized || !handle.valid() || !mesh.valid() {
    return false
  }
  let material_key = match mesh.material {
    None => -1
    Some(material_handle) => material_handle.id
  }
  let uploaded = opengl_upload_mesh(
    handle.id,
    material_key,
    mesh.vertices,
    mesh.vertex_count,
    mesh.floats_per_vertex,
  )
  if uploaded {
    append_key(self.uploaded_mesh_keys, handle.id)
  }
  uploaded
}

///|
pub fn Renderer::remove_mesh(
  self : Renderer,
  handle : @renderer.MeshHandle,
) -> Unit {
  if self.initialized && handle.valid() {
    opengl_remove_mesh(handle.id)
  }
  if handle.valid() {
    self.uploaded_mesh_keys = remove_key(self.uploaded_mesh_keys, handle.id)
  }
}

///|
pub fn Renderer::draw_frame(
  self : Renderer,
  frame : @renderer.FrameData,
) -> Unit {
  if self.initialized && frame.valid() {
    opengl_draw_frame(
      frame.view_projection,
      frame.viewport_width,
      frame.viewport_height,
    )
  }
}

///|
pub fn Renderer::uploaded_mesh_count(self : Renderer) -> Int {
  self.uploaded_mesh_keys.length()
}

///|
pub fn Renderer::uploaded_texture_count(self : Renderer) -> Int {
  self.uploaded_texture_keys.length()
}

///|
pub fn Renderer::uploaded_material_count(self : Renderer) -> Int {
  self.uploaded_material_keys.length()
}

///|
pub fn Renderer::shutdown(self : Renderer) -> Unit {
  if self.initialized {
    opengl_shutdown()
    self.initialized = false
    self.uploaded_mesh_keys = []
    self.uploaded_texture_keys = []
    self.uploaded_material_keys = []
  }
}
