/// A backend-neutral mesh resource name.
pub(all) struct MeshHandle {
  id : Int
}

/// A backend-neutral texture resource name.
pub(all) struct TextureHandle {
  id : Int
}

/// A backend-neutral material resource name.
pub(all) struct MaterialHandle {
  id : Int
}

/// RGBA8 image data owned by the MoonBit side until upload.
pub(all) struct TextureData {
  width : Int
  height : Int
  rgba : Bytes
}

/// Fragment alpha behavior requested by a material.
pub(all) enum AlphaMode {
  Opaque
  Blend
}

/// Material data consumed by a rendering backend.
pub(all) struct MaterialData {
  texture : TextureHandle?
  tint : FixedArray[Float]
  alpha_mode : AlphaMode
}

/// A renderable interleaved vertex buffer.
///
/// The first runtime format contains position (3), UV (2), and shade (1).
pub(all) struct MeshData {
  vertices : FixedArray[Float]
  vertex_count : Int
  floats_per_vertex : Int
  material : MaterialHandle?
}

/// A frame's camera transform.
pub(all) struct FrameData {
  view_projection : FixedArray[Float]
  viewport_width : Int
  viewport_height : Int
}

///|
pub fn MeshHandle::valid(self : MeshHandle) -> Bool {
  self.id >= 0
}

///|
pub fn TextureHandle::valid(self : TextureHandle) -> Bool {
  self.id >= 0
}

///|
pub fn MaterialHandle::valid(self : MaterialHandle) -> Bool {
  self.id >= 0
}

///|
pub fn TextureData::valid(self : TextureData) -> Bool {
  self.width > 0 &&
    self.height > 0 &&
    self.rgba.length() == self.width * self.height * 4
}

///|
pub fn MaterialData::valid(self : MaterialData) -> Bool {
  let texture_valid = match self.texture {
    None => true
    Some(handle) => handle.valid()
  }
  self.tint.length() == 4 && texture_valid
}

///|
pub fn MeshData::valid(self : MeshData) -> Bool {
  let material_valid = match self.material {
    None => true
    Some(handle) => handle.valid()
  }
  self.vertex_count >= 0 &&
    self.floats_per_vertex == 6 &&
    self.vertex_count * self.floats_per_vertex == self.vertices.length() &&
    material_valid
}

///|
pub fn FrameData::valid(self : FrameData) -> Bool {
  self.view_projection.length() == 16 &&
    self.viewport_width > 0 &&
    self.viewport_height > 0
}
