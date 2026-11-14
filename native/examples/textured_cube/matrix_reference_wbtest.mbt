// Reference matrix helpers for the demo's whitebox tests and benchmarks.
//
// The runtime frame path (`demo_frame` in main.mbt) composes the model-view
// matrix directly instead of multiplying temporary matrices, so these helpers
// exist only to cross-check it: the "demo_frame composes the same matrix as
// the matrix helpers" test rebuilds the same transform the slow way and
// compares every element.

///|
fn identity_matrix() -> FixedArray[Float] {
  FixedArray::makei(16, index => if index % 5 == 0 { 1.0 } else { 0.0 })
}

///|
fn rotation_x(angle : Double) -> FixedArray[Float] {
  let result = identity_matrix()
  let cosine = Float::from_double(@stdmath.cos(angle))
  let sine = Float::from_double(@stdmath.sin(angle))
  result[5] = cosine
  result[6] = sine
  result[9] = -sine
  result[10] = cosine
  result
}

///|
fn rotation_y(angle : Double) -> FixedArray[Float] {
  let result = identity_matrix()
  let cosine = Float::from_double(@stdmath.cos(angle))
  let sine = Float::from_double(@stdmath.sin(angle))
  result[0] = cosine
  result[2] = -sine
  result[8] = sine
  result[10] = cosine
  result
}

///|
fn translation(x : Float, y : Float, z : Float) -> FixedArray[Float] {
  let result = identity_matrix()
  result[12] = x
  result[13] = y
  result[14] = z
  result
}

///|
fn perspective(aspect : Double) -> FixedArray[Float] {
  let near = 0.1
  let far = 100.0
  let fov = @stdmath.PI * 70.0 / 180.0
  let focal = 1.0 / @stdmath.tan(fov / 2.0)
  let result : FixedArray[Float] = FixedArray::make(16, 0.0)
  result[0] = Float::from_double(focal / aspect)
  result[5] = Float::from_double(focal)
  result[10] = Float::from_double((far + near) / (near - far))
  result[11] = -1.0
  result[14] = Float::from_double(2.0 * far * near / (near - far))
  result
}
