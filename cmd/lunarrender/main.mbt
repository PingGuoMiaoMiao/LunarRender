///|
const WindowWidth : Int = 960

///|
const WindowHeight : Int = 540

///|
const MouseSensitivity : Double = 0.0025

///|
const KeyEscape : Int = 256

///|
const KeySpace : Int = 32

///|
const KeyW : Int = 87

///|
const KeyA : Int = 65

///|
const KeyS : Int = 83

///|
const KeyD : Int = 68

///|
const KeyF7 : Int = 296

///|
const KeyF8 : Int = 297

///|
const MouseButtonLeft : Int = 0

///|
const MouseButtonRight : Int = 1

///|
const KeyEnter : Int = 257

///|
const KeyBackspace : Int = 259

///|
const KeySlash : Int = 47

///|
const KeyMinus : Int = 45

///|
const MaxCommandLength : Int = 80

///|
pub fn models_enabled() -> Bool {
  true
}

///|
fn clicked(previous : Bool, current : Bool) -> Bool {
  current && !previous
}

///|
fn command_line_seed() -> Int64 {
  let args = @env.args()
  for i = 0; i + 1 < args.length(); i = i + 1 {
    if args[i] == "--seed" {
      return try @string.parse_int64(args[i + 1]) catch {
        _ => @world.DefaultSeed
      } noraise {
        value => value
      }
    }
  }
  @world.DefaultSeed
}

///|
fn chunk_file_path(position : @world.ChunkPos) -> String {
  "saves/default/chunks/" + position.x.to_string() + "_" + position.z.to_string() + ".bin"
}

///|
fn load_saved_metadata(seed : Int64) -> @save.SaveMetadata? {
  let path = "saves/default/world.json"
  if !@native.file_exists(path) {
    return None
  }
  let bytes = @native.file_read(path)
  match @save.decode_world_json(bytes) {
    Some(metadata) => if metadata.seed == seed { Some(metadata) } else { None }
    None => None
  }
}

///|
fn restore_saved_chunk(
  world : @world.WorldStore,
  position : @world.ChunkPos,
  allowed : Bool,
) -> Unit {
  if !allowed {
    return
  }
  let path = chunk_file_path(position)
  if !@native.file_exists(path) {
    return
  }
  let bytes = @native.file_read(path)
  match @save.decode_chunk(bytes) {
    Some((decoded_position, chunk)) => {
      if decoded_position == position {
        ignore(world.restore_chunk(position, chunk))
      }
    }
    None => ()
  }
}

///|
fn load_resource_pack(name : String) -> Bool {
  let root = @assets.resource_pack_path(name)
  if root.length() == 0 {
    return false
  }
  let manifest_path = root + "/pack.json"
  if !@native.file_exists(manifest_path) {
    return false
  }
  let manifest_bytes = @native.file_read(manifest_path)
  match @assets.parse_manifest(manifest_bytes) {
    Some(manifest) =>
      if !@assets.validate_manifest(manifest) {
        false
      } else {
        let ids = ["stone", "dirt", "grass_side", "grass_top"]
        let images : Array[@assets.ImageData] = []
        let mut valid = true
        for id in ids {
          match manifest.textures.get(id) {
            Some(relative_path) => {
              let image_path = root + "/" + relative_path
              if !@native.file_exists(image_path) {
                valid = false
              } else {
                let image_bytes = @native.image_decode_rgba(image_path)
                match @assets.image_from_native_bytes(image_bytes) {
                  Some(image) => images.push(image)
                  None => valid = false
                }
              }
            }
            None => valid = false
          }
        }
        if !valid {
          false
        } else {
          match @assets.build_texture_atlas(images) {
            Some(atlas) => @native.gl_replace_texture_atlas(atlas.width, atlas.height, atlas.rgba)
            None => false
          }
        }
      }
    None => false
  }
}

///|
fn ensure_save_directories() -> Bool {
  @native.save_ensure_directory("saves") &&
    @native.save_ensure_directory("saves/default") &&
    @native.save_ensure_directory("saves/default/chunks")
}

///|
fn write_atomic(path : String, bytes : Bytes) -> Bool {
  let temporary = path + ".tmp"
  @native.save_write_temp(temporary, bytes) && @native.save_replace_temp(temporary, path)
}

///|
fn save_game(
  world : @world.WorldStore,
  player : @physics.PlayerState,
  hotbar : @inventory.Hotbar,
  pack_name : String,
) -> Bool {
  if !ensure_save_directories() {
    println("LunarRender: failed to create save directories")
    return false
  }
  let dirty = world.dirty_positions()
  let mut chunks_ok = true
  for position in dirty {
    match world.chunk(position) {
      Some(chunk) =>
        if !write_atomic(chunk_file_path(position), @save.encode_chunk(position, chunk)) {
          chunks_ok = false
        } else {
          world.clear_dirty(position)
        }
      None => ()
    }
  }
  let metadata = @save.SaveMetadata::{
    format_version: 1,
    seed: world.seed(),
    pack_name,
    player,
    hotbar,
  }
  let metadata_ok = write_atomic("saves/default/world.json", @save.encode_world_json(metadata))
  chunks_ok && metadata_ok
}

///|
fn player_input(camera : @math.Camera, jump_pressed : Bool) -> @physics.PlayerInput {
  let forward = camera.forward()
  let horizontal_forward = @math.Vec3(forward.0, 0.0, forward.2).normalized()
  let right = camera.right()
  let horizontal_right = @math.Vec3(right.0, 0.0, right.2).normalized()
  let mut forward_amount = 0.0
  let mut right_amount = 0.0
  if @native.window_key_down(KeyW) {
    forward_amount = forward_amount + 1.0
  }
  if @native.window_key_down(KeyS) {
    forward_amount = forward_amount - 1.0
  }
  if @native.window_key_down(KeyD) {
    right_amount = right_amount + 1.0
  }
  if @native.window_key_down(KeyA) {
    right_amount = right_amount - 1.0
  }
  let movement = horizontal_forward.scale(forward_amount) + horizontal_right.scale(right_amount)
  @physics.PlayerInput::{
    move_forward: movement.2,
    move_right: movement.0,
    jump_pressed,
  }
}

///|
fn apply_block_edit(
  world : @world.WorldStore,
  camera : @math.Camera,
  block_to_place : @world.BlockKind,
  break_pressed : Bool,
  place_pressed : Bool,
) -> Bool {
  if !break_pressed && !place_pressed {
    return false
  }
  match @raycast.raycast(world, camera.position(), camera.forward(), 8.0) {
    None => false
    Some(hit) =>
      if break_pressed {
        match world.set_block(hit.block_x, hit.block_y, hit.block_z, @world.BlockKind::Air) {
          @world.EditResult::Changed => true
          _ => false
        }
      } else {
        match world.set_block(hit.place_x, hit.place_y, hit.place_z, block_to_place) {
          @world.EditResult::Changed => true
          _ => false
        }
      }
  }
}

///|
fn sync_camera_to_player(camera : @math.Camera, player : @physics.PlayerState) -> Unit {
  let target = player.position + @math.Vec3(0.0, @physics.EyeHeight, 0.0)
  camera.move_by(target - camera.position())
}

///|
fn load_player_skin() -> Bool {
  let path = "assets/textures/steve.png"
  if !@native.file_exists(path) {
    return false
  }
  let image_bytes = @native.image_decode_rgba(path)
  match @assets.image_from_native_bytes(image_bytes) {
    Some(image) => @native.gl_replace_player_texture(image.width, image.height, image.rgba)
    None => false
  }
}

///|
fn player_yaw_from_movement(
  input : @physics.PlayerInput,
  fallback : Double,
) -> Double {
  if input.move_forward == 0.0 && input.move_right == 0.0 {
    fallback
  } else {
    @stdmath.atan2(input.move_forward, input.move_right)
  }
}

///|
fn command_key_pressed(previous : FixedArray[Bool], key : Int) -> Bool {
  @native.window_key_down(key) && !previous[key]
}

///|
fn command_key_char(key : Int) -> String? {
  let code = if key >= 65 && key <= 90 {
    key + 32
  } else if (key >= 48 && key <= 57) || key == KeySlash || key == KeyMinus || key == KeySpace {
    key
  } else {
    return None
  }
  match code.to_char() {
    Some(value) => Some(value.to_string())
    None => None
  }
}

///|
fn execute_command(
  command : @command.Command,
  world : @world.WorldStore,
  player : @physics.PlayerState,
  hotbar : @inventory.Hotbar,
  active_pack : String,
) -> String {
  match command {
    @command.Command::Help => "help tp setblock save seed view; esc close"
    @command.Command::Seed => "seed " + world.seed().to_string()
    @command.Command::Save => if save_game(world, player, hotbar, active_pack) {
      "save ok"
    } else {
      "save failed"
    }
    @command.Command::Teleport(x, y, z) => {
      player.position = @math.Vec3(x.to_double(), y.to_double(), z.to_double())
      player.velocity = @math.Vec3(0.0, 0.0, 0.0)
      player.on_ground = false
      player.jump_requested = false
      "teleported"
    }
    @command.Command::SetBlock(x, y, z, kind) =>
      match world.set_block(x, y, z, kind) {
        @world.EditResult::Changed => "setblock changed"
        @world.EditResult::Occupied => "setblock occupied"
        @world.EditResult::Unloaded => "setblock unloaded"
        @world.EditResult::OutOfRange => "setblock out of range"
      }
    @command.Command::ViewFirst => "view first"
    @command.Command::ViewThird => "view third"
  }
}

///|
fn update_command_keys(previous : FixedArray[Bool]) -> Unit {
  for key = 0; key < previous.length(); key = key + 1 {
    previous[key] = @native.window_key_down(key)
  }
}

///|
fn sync_third_person_camera(
  world : @world.WorldStore,
  camera : @math.Camera,
  player : @physics.PlayerState,
) -> Unit {
  let target = player.position + @math.Vec3(0.0, 1.0, 0.0)
  let direction = camera.forward().scale(-1.0)
  let distance = match @raycast.raycast(world, target, direction, 4.0) {
    Some(hit) => (hit.distance - 0.35).max(0.65)
    None => 4.0
  }
  let desired = target + direction.scale(distance)
  camera.move_by(desired - camera.position())
}

///|
fn rebuild_dirty_chunks(world : @world.WorldStore, renderer : @renderer.Renderer) -> Bool {
  let mut ok = true
  for position in world.take_dirty_chunks() {
    if world.is_loaded(position) {
      let mesh = @mesh.build_chunk_mesh(world, position)
      if !renderer.upload_chunk(position, mesh) {
        ok = false
      }
    }
  }
  ok
}

///|
fn preload_spawn_chunks(world : @world.WorldStore, center : @world.ChunkPos) -> Unit {
  for dx = -1; dx <= 1; dx = dx + 1 {
    for dz = -1; dz <= 1; dz = dz + 1 {
      world.load_chunk(@world.ChunkPos::{ x: center.x + dx, z: center.z + dz })
    }
  }
}

///|
fn main {
  let seed = command_line_seed()
  let saved_metadata = load_saved_metadata(seed)
  let default_position = @math.Vec3(
    8.0,
    (@world.terrain_height(seed, 8, 8) + 1).to_double(),
    8.0,
  )
  let spawn_position = match saved_metadata {
    Some(metadata) => metadata.player.position
    None => default_position
  }
  let hotbar = match saved_metadata {
    Some(metadata) => metadata.hotbar
    None => @inventory.Hotbar::new()
  }
  let mut active_pack = match saved_metadata {
    Some(metadata) => metadata.pack_name
    None => "default"
  }

  if !@native.window_create(WindowWidth, WindowHeight) {
    println("LunarRender: failed to create GLFW window")
    return
  }
  if !@native.gl_init() {
    println("LunarRender: failed to initialize OpenGL")
    @native.window_destroy()
    return
  }
  if !load_player_skin() {
    println("LunarRender: failed to load assets/textures/steve.png")
    @native.gl_destroy_renderer()
    @native.window_destroy()
    return
  }
  if !load_resource_pack(active_pack) {
    active_pack = "default"
    ignore(load_resource_pack(active_pack))
  }

  let world = @world.WorldStore::new(seed)
  let streamer = @world.default_chunk_streamer()
  let (spawn_chunk_x, _) = @world.world_to_chunk(spawn_position.0.floor().to_int())
  let (spawn_chunk_z, _) = @world.world_to_chunk(spawn_position.2.floor().to_int())
  preload_spawn_chunks(world, @world.ChunkPos::{ x: spawn_chunk_x, z: spawn_chunk_z })
  let entities = @entity.EntityStore::new()
  let player_id = entities.create_player(spawn_position)
  let player = match entities.get_player(player_id) {
    Some(value) => value
    None => {
      println("LunarRender: failed to create local player")
      @native.window_destroy()
      return
    }
  }
  match saved_metadata {
    Some(metadata) => {
      player.velocity = metadata.player.velocity
      player.on_ground = metadata.player.on_ground
      player.jump_requested = metadata.player.jump_requested
    }
    None => player.on_ground = true
  }
  let restore_attempted = @hashset.HashSet([], capacity=128)
  let restore_allowed = saved_metadata is Some(_)
  let camera = @math.Camera::new(spawn_position + @math.Vec3(0.0, @physics.EyeHeight, 0.0))
  camera.look(0.0, -0.14)
  let renderer = @renderer.Renderer::new()
  let mut previous_time = @native.window_time_seconds()
  let mut previous_left = false
  let mut previous_right = false
  let mut previous_jump = false
  let mut previous_f7 = false
  let mut previous_f8 = false
  let mut third_person = false
  let mut player_yaw = camera.yaw_angle()
  let previous_number_keys : FixedArray[Bool] = FixedArray::make(9, false)
  let previous_keys : FixedArray[Bool] = FixedArray::make(512, false)
  let mut command_open = false
  let mut command_text = ""
  let mut command_status = ""

  while !@native.window_should_close() {
    @native.window_poll_events()
    let escape = @native.window_key_down(KeyEscape)
    let current_time = @native.window_time_seconds()
    let delta_time = (current_time - previous_time).max(0.0).min(0.05)
    previous_time = current_time

    let was_command_open = command_open
    let slash_pressed = command_key_pressed(previous_keys, KeySlash)
    let mut command_interaction = was_command_open || slash_pressed
    let mut parsed_command : @command.Command? = None
    if was_command_open {
      if command_key_pressed(previous_keys, KeyEscape) {
        command_open = false
      } else if command_key_pressed(previous_keys, KeyEnter) {
        parsed_command = @command.parse(command_text)
        command_open = false
        command_text = ""
        command_interaction = true
      } else if command_key_pressed(previous_keys, KeyBackspace) {
        if command_text.length() > 1 {
          let chars = command_text.to_array()
          command_text = String::from_array(chars[:chars.length() - 1])
        }
      } else if command_text.length() < MaxCommandLength {
        for key = 0; key < 128; key = key + 1 {
          if command_key_pressed(previous_keys, key) {
            match command_key_char(key) {
              Some(value) => command_text = command_text + value
              None => ()
            }
          }
        }
      }
    } else if slash_pressed {
      command_open = true
      command_text = "/"
      command_status = ""
    }

    match parsed_command {
      Some(@command.Command::ViewFirst) => {
        third_person = false
        command_status = "view first"
      }
      Some(@command.Command::ViewThird) => {
        third_person = true
        command_status = "view third"
      }
      Some(command) => command_status = execute_command(command, world, player, hotbar, active_pack)
      None => if was_command_open && !command_open && command_text == "" {
        command_status = "unknown command"
      }
    }

    if !command_interaction {
      let mouse_x = @native.window_cursor_delta_x()
      let mouse_y = @native.window_cursor_delta_y()
      camera.look(-mouse_x * MouseSensitivity, -mouse_y * MouseSensitivity)
    }

    if !command_interaction {
      for slot = 0; slot < 9; slot = slot + 1 {
        let key_down = @native.window_key_down(49 + slot)
        if key_down && !previous_number_keys[slot] {
          ignore(hotbar.select(slot))
        }
        previous_number_keys[slot] = key_down
      }
    }
    let f7 = @native.window_key_down(KeyF7)
    if !command_interaction && clicked(previous_f7, f7) {
      let next_pack = if active_pack == "default" { "warm" } else { "default" }
      if load_resource_pack(next_pack) {
        active_pack = next_pack
      }
    }
    previous_f7 = f7

    let f8 = @native.window_key_down(KeyF8)
    if !command_interaction && clicked(previous_f8, f8) {
      third_person = !third_person
    }
    previous_f8 = f8

    let jump = @native.window_key_down(KeySpace)
    let movement_input = if command_interaction {
      @physics.PlayerInput::{ move_forward: 0.0, move_right: 0.0, jump_pressed: false }
    } else {
      player_input(camera, clicked(previous_jump, jump))
    }
    if !command_interaction && (movement_input.move_forward != 0.0 || movement_input.move_right != 0.0) {
      player_yaw = player_yaw_from_movement(movement_input, player_yaw)
    }
    player.step(world, movement_input, delta_time)
    previous_jump = jump
    if third_person {
      sync_third_person_camera(world, camera, player)
    } else {
      sync_camera_to_player(camera, player)
    }

    let (center_x, _) = @world.world_to_chunk(player.position.0.floor().to_int())
    let (center_z, _) = @world.world_to_chunk(player.position.2.floor().to_int())
    let center = @world.ChunkPos::{ x: center_x, z: center_z }
    let old_loaded = world.loaded_positions()
    streamer.update(world, center)
    old_loaded.each(position => {
      if !world.is_loaded(position) {
        renderer.remove_chunk(position)
        restore_attempted.remove(position)
      }
    })
    ignore(streamer.process(world, 2))

    world.loaded_positions().each(position => {
      if !restore_attempted.contains(position) {
        restore_attempted.add(position)
        restore_saved_chunk(world, position, restore_allowed)
      }
    })

    let left = @native.window_mouse_button_down(MouseButtonLeft)
    let right = @native.window_mouse_button_down(MouseButtonRight)
    let edited = if command_interaction {
      false
    } else {
      apply_block_edit(
        world,
        camera,
        hotbar.selected_block(),
        clicked(previous_left, left),
        clicked(previous_right, right),
      )
    }
    previous_left = left
    previous_right = right
    if edited && !rebuild_dirty_chunks(world, renderer) {
      println("LunarRender: failed to upload edited chunk mesh")
      break
    }
    if !rebuild_dirty_chunks(world, renderer) {
      println("LunarRender: failed to upload streamed chunk mesh")
      break
    }

    let view_projection = camera.view_projection(
      WindowWidth.to_double() / WindowHeight.to_double(),
    )
    let walk_amount = (
      movement_input.move_forward * movement_input.move_forward +
        movement_input.move_right * movement_input.move_right
    ).sqrt().min(1.0)
    let walk_phase = current_time * 8.0
    if third_person {
      let player_mesh = @mesh.build_player_mesh_with_pose(
        player.position,
        player_yaw,
        @mesh.PlayerPose::{ limb_swing: walk_phase, limb_amount: walk_amount },
      )
      if !renderer.upload_player(player_mesh) {
        println("LunarRender: failed to upload player mesh")
        break
      }
    } else {
      let forward = camera.forward()
      let right = camera.right()
      let up = right.cross(forward).normalized()
      let arm_mesh = @mesh.build_first_person_arm_mesh(
        camera.position(),
        right,
        up,
        forward,
        walk_phase,
        walk_amount,
      )
      if !renderer.upload_player(arm_mesh) {
        println("LunarRender: failed to upload first-person arm mesh")
        break
      }
    }
    let selection = match @raycast.raycast(world, camera.position(), camera.forward(), 8.0) {
      Some(hit) => Some(@render.BlockSelection::{
        block_x: hit.block_x,
        block_y: hit.block_y,
        block_z: hit.block_z,
      })
      None => None
    }
    renderer.draw_frame(@render.RenderSnapshot::{
      view_projection: view_projection,
      selection: selection,
      show_player: true,
      hud: @render.HudSnapshot::{
        selected_slot: hotbar.selected,
        command_open: command_open,
        command_line: command_text,
        command_status: command_status,
      },
    })
    @native.window_swap_buffers()
    update_command_keys(previous_keys)
    if escape && !was_command_open {
      break
    }
  }

  ignore(save_game(world, player, hotbar, active_pack))
  renderer.shutdown()
  @native.window_destroy()
}
