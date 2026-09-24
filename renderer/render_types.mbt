/// A renderable interleaved vertex buffer.
///
/// The first runtime format contains position (3), UV (2), and shade (1).
pub(all) struct MeshData {
  vertices : FixedArray[Float]
  vertex_count : Int
  floats_per_vertex : Int
}

/// A frame's camera transform.
pub(all) struct FrameData {
  view_projection : FixedArray[Float]
}

///| 
pub fn MeshData::valid(self : MeshData) -> Bool {
  self.vertex_count >= 0 &&
    self.floats_per_vertex == 6 &&
    self.vertex_count * self.floats_per_vertex == self.vertices.length()
}

///| 
pub fn FrameData::valid(self : FrameData) -> Bool {
  self.view_projection.length() == 16
}
