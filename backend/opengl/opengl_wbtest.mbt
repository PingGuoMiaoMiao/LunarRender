///|
test "backend rejects invalid mesh data before native upload" {
  let invalid = @renderer.MeshData::{
    vertices: FixedArray::make(5, 0.0),
    vertex_count: 1,
    floats_per_vertex: 6,
    material: None,
  }
  assert_false(invalid.valid())
}

///|
test "backend starts with zero uploaded meshes in its state model" {
  let renderer = Renderer::{
    initialized: false,
    uploaded_mesh_keys: [],
    uploaded_texture_keys: [],
    uploaded_material_keys: [],
  }
  assert_eq(renderer.uploaded_mesh_count(), 0)
  assert_eq(renderer.uploaded_texture_count(), 0)
  assert_eq(renderer.uploaded_material_count(), 0)
}
