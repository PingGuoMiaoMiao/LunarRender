///|
test (b : @bench.T) {
  b.bench(name="demo_frame", fn() { b.keep(demo_frame(1.234, 1280, 720)) })
}

///|
test (b : @bench.T) {
  b.bench(name="demo_mesh", fn() { b.keep(demo_mesh()) })
}

///|
test (b : @bench.T) {
  let left = identity_matrix()
  let right = rotation_x(0.5)
  b.bench(name="mat4_mul", fn() { b.keep(mat4_mul(left, right)) })
}
