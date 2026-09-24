///|
test "framebuffer dimensions are zero before a window exists" {
  assert_eq(window_framebuffer_width(), 0)
  assert_eq(window_framebuffer_height(), 0)
}
