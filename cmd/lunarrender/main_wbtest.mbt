///|
test "demo mesh uses the backend-neutral six-float vertex format" {
  let mesh = demo_mesh()
  assert_true(mesh.valid())
  assert_eq(mesh.vertex_count, 3)
  assert_eq(mesh.floats_per_vertex, 6)
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
