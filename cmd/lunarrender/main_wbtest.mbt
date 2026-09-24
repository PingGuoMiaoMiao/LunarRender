///|
test "demo mesh uses the backend-neutral six-float vertex format" {
  let mesh = demo_mesh()
  assert_true(mesh.valid())
  assert_eq(mesh.vertex_count, 36)
  assert_eq(mesh.floats_per_vertex, 6)
}

///|
test "checkerboard texture has exact rgba8 size" {
  let texture = checkerboard_texture()
  assert_eq(texture.width, 8)
  assert_eq(texture.height, 8)
  assert_eq(texture.rgba.length(), 8 * 8 * 4)
  assert_true(texture.valid())
}

///|
test "demo mesh is a six-face cube" {
  let mesh = demo_mesh()
  assert_eq(mesh.vertex_count, 36)
  assert_eq(mesh.floats_per_vertex, 6)
  assert_eq(mesh.vertices.length(), 36 * 6)
  assert_true(mesh.valid())
}

///|
test "demo frame follows supplied viewport" {
  let frame = demo_frame(0.0, 1280, 720)
  assert_eq(frame.viewport_width, 1280)
  assert_eq(frame.viewport_height, 720)
  assert_eq(frame.view_projection.length(), 16)
  assert_true(frame.valid())
}

///|
test "demo frame uses a sixteen-value identity matrix" {
  let matrix = identity_matrix()
  assert_eq(matrix.length(), 16)
  assert_eq(matrix[0], 1.0)
  assert_eq(matrix[5], 1.0)
  assert_eq(matrix[10], 1.0)
  assert_eq(matrix[15], 1.0)
  assert_eq(matrix[1], 0.0)
}

///|
test "self test mode is controlled by an explicit argument" {
  assert_false(self_test_mode())
}
