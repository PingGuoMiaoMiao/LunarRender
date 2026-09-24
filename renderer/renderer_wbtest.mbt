///|
test "renderer starts without an uploaded mesh" {
  let renderer = Renderer::new()
  assert_eq(renderer.vertex_count(), 0)
}
