///|
test "click edge is true only on press" {
  assert_eq(clicked(false, true), true)
  assert_eq(clicked(true, true), false)
  assert_eq(clicked(false, false), false)
}

///|
test "singleplayer client enables the Steve skin path" {
  assert_true(models_enabled())
}

///|
test "startup preloads the center and its eight visible neighbor chunks" {
  let world = @world.WorldStore::new(@world.DefaultSeed)
  let center = @world.ChunkPos::{ x: -2, z: 3 }
  preload_spawn_chunks(world, center)
  assert_eq(world.loaded_positions().length(), 9)
  for dx = -1; dx <= 1; dx = dx + 1 {
    for dz = -1; dz <= 1; dz = dz + 1 {
      assert_true(world.is_loaded(@world.ChunkPos::{ x: center.x + dx, z: center.z + dz }))
    }
  }
}

///|
test "third person camera keeps the player in the view direction" {
  let camera = @math.Camera::new(@math.Vec3(0.0, 10.0, 0.0))
  camera.look(0.0, -0.2)
  let player = @physics.PlayerState::new(@math.Vec3(0.0, 2.0, 0.0))
  let world = @world.WorldStore::new(0L)
  sync_third_person_camera(world, camera, player)
  let target = player.position + @math.Vec3(0.0, 1.0, 0.0)
  let target_direction = (target - camera.position()).normalized()
  assert_true(target_direction.dot(camera.forward()) > 0.999)
}

///|
test "third person camera stops before a solid block behind the player" {
  let world = @world.WorldStore::new(0L)
  world.load_chunk(@world.ChunkPos::{ x: 0, z: 0 })
  ignore(world.set_block(0, 3, 2, @world.BlockKind::Stone))
  let camera = @math.Camera::new(@math.Vec3(0.0, 10.0, 0.0))
  camera.look(0.0, -0.2)
  let player = @physics.PlayerState::new(@math.Vec3(0.0, 2.0, 0.0))
  sync_third_person_camera(world, camera, player)
  assert_true(camera.position().2 < 2.0)
}

///|
test "player yaw follows movement and remains stable while idle" {
  let fallback = -@stdmath.PI / 2.0
  let forward = @physics.PlayerInput::{ move_forward: -1.0, move_right: 0.0, jump_pressed: false }
  let right = @physics.PlayerInput::{ move_forward: 0.0, move_right: 1.0, jump_pressed: false }
  let idle = @physics.PlayerInput::{ move_forward: 0.0, move_right: 0.0, jump_pressed: false }
  assert_eq(player_yaw_from_movement(forward, fallback), -@stdmath.PI / 2.0)
  assert_eq(player_yaw_from_movement(right, fallback), 0.0)
  assert_eq(player_yaw_from_movement(idle, fallback), fallback)
}
