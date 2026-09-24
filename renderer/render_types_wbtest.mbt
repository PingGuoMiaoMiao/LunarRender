///|
test "mesh data accepts the first runtime vertex format" {
  let mesh = MeshData::{
    vertices: FixedArray::make(12, 0.0),
    vertex_count: 2,
    floats_per_vertex: 6,
    material: None,
  }
  assert_true(mesh.valid())
}

///|
test "mesh data accepts an empty buffer" {
  let mesh = MeshData::{
    vertices: FixedArray::make(0, 0.0),
    vertex_count: 0,
    floats_per_vertex: 6,
    material: None,
  }
  assert_true(mesh.valid())
}

///|
test "mesh data rejects a negative vertex count" {
  let mesh = MeshData::{
    vertices: FixedArray::make(0, 0.0),
    vertex_count: -1,
    floats_per_vertex: 6,
    material: None,
  }
  assert_false(mesh.valid())
}

///|
test "mesh data rejects an unsupported stride" {
  let mesh = MeshData::{
    vertices: FixedArray::make(8, 0.0),
    vertex_count: 2,
    floats_per_vertex: 4,
    material: None,
  }
  assert_false(mesh.valid())
}

///|
test "mesh data rejects an array length mismatch" {
  let mesh = MeshData::{
    vertices: FixedArray::make(11, 0.0),
    vertex_count: 2,
    floats_per_vertex: 6,
    material: None,
  }
  assert_false(mesh.valid())
}

///|
test "frame data requires sixteen matrix values" {
  let valid = FrameData::{ view_projection: FixedArray::make(16, 0.0), viewport_width: 960, viewport_height: 540 }
  let invalid = FrameData::{ view_projection: FixedArray::make(15, 0.0), viewport_width: 960, viewport_height: 540 }
  assert_true(valid.valid())
  assert_false(invalid.valid())
}

///|
test "resource handles reject negative ids" {
  assert_false(MeshHandle::{ id: -1 }.valid())
  assert_false(TextureHandle::{ id: -1 }.valid())
  assert_false(MaterialHandle::{ id: -1 }.valid())
  assert_true(MeshHandle::{ id: 0 }.valid())
}

///|
test "texture data requires exact rgba8 length" {
  let valid = TextureData::{ width: 2, height: 1, rgba: Bytes::from_array([255, 0, 0, 255, 0, 255, 0, 255]) }
  let short = TextureData::{ width: 2, height: 1, rgba: Bytes::from_array([255, 0, 0, 255]) }
  let zero = TextureData::{ width: 0, height: 1, rgba: Bytes::from_array([]) }
  assert_true(valid.valid())
  assert_false(short.valid())
  assert_false(zero.valid())
}

///|
test "material validates tint and optional texture handle" {
  let valid = MaterialData::{ texture: Some(TextureHandle::{ id: 4 }), tint: [1.0, 1.0, 1.0, 1.0], alpha_mode: Opaque }
  let bad_tint = MaterialData::{ texture: None, tint: [1.0, 1.0, 1.0], alpha_mode: Opaque }
  let bad_texture = MaterialData::{ texture: Some(TextureHandle::{ id: -1 }), tint: [1.0, 1.0, 1.0, 1.0], alpha_mode: Blend }
  assert_true(valid.valid())
  assert_false(bad_tint.valid())
  assert_false(bad_texture.valid())
}

///|
test "mesh and frame validate material and viewport" {
  let mesh = MeshData::{ vertices: FixedArray::make(6, 0.0), vertex_count: 1, floats_per_vertex: 6, material: Some(MaterialHandle::{ id: 2 }) }
  let bad_mesh = MeshData::{ vertices: FixedArray::make(6, 0.0), vertex_count: 1, floats_per_vertex: 6, material: Some(MaterialHandle::{ id: -1 }) }
  let frame = FrameData::{ view_projection: FixedArray::make(16, 0.0), viewport_width: 960, viewport_height: 540 }
  let bad_frame = FrameData::{ view_projection: FixedArray::make(16, 0.0), viewport_width: 0, viewport_height: 540 }
  assert_true(mesh.valid())
  assert_false(bad_mesh.valid())
  assert_true(frame.valid())
  assert_false(bad_frame.valid())
}
