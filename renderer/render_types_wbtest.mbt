///|
test "mesh data accepts the first runtime vertex format" {
  let mesh = MeshData::{
    vertices: FixedArray::make(12, 0.0),
    vertex_count: 2,
    floats_per_vertex: 6,
  }
  assert_true(mesh.valid())
}

///|
test "mesh data accepts an empty buffer" {
  let mesh = MeshData::{
    vertices: FixedArray::make(0, 0.0),
    vertex_count: 0,
    floats_per_vertex: 6,
  }
  assert_true(mesh.valid())
}

///|
test "mesh data rejects a negative vertex count" {
  let mesh = MeshData::{
    vertices: FixedArray::make(0, 0.0),
    vertex_count: -1,
    floats_per_vertex: 6,
  }
  assert_false(mesh.valid())
}

///|
test "mesh data rejects an unsupported stride" {
  let mesh = MeshData::{
    vertices: FixedArray::make(8, 0.0),
    vertex_count: 2,
    floats_per_vertex: 4,
  }
  assert_false(mesh.valid())
}

///|
test "mesh data rejects an array length mismatch" {
  let mesh = MeshData::{
    vertices: FixedArray::make(11, 0.0),
    vertex_count: 2,
    floats_per_vertex: 6,
  }
  assert_false(mesh.valid())
}

///|
test "frame data requires sixteen matrix values" {
  let valid = FrameData::{ view_projection: FixedArray::make(16, 0.0) }
  let invalid = FrameData::{ view_projection: FixedArray::make(15, 0.0) }
  assert_true(valid.valid())
  assert_false(invalid.valid())
}
