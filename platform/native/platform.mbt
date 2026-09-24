///|
pub extern "C" fn window_create(width : Int, height : Int) -> Bool = "lunarrender_window_create"

///|
pub extern "C" fn window_should_close() -> Bool = "lunarrender_window_should_close"

///|
pub extern "C" fn window_poll_events() -> Unit = "lunarrender_window_poll_events"

///|
pub extern "C" fn window_swap_buffers() -> Unit = "lunarrender_window_swap_buffers"

///|
pub extern "C" fn window_destroy() -> Unit = "lunarrender_window_destroy"

///|
pub extern "C" fn window_time_seconds() -> Double = "lunarrender_window_time_seconds"

///|
pub extern "C" fn window_key_down(key : Int) -> Bool = "lunarrender_window_key_down"

///|
pub extern "C" fn window_mouse_button_down(button : Int) -> Bool = "lunarrender_window_mouse_button_down"

///|
pub extern "C" fn window_cursor_delta_x() -> Double = "lunarrender_window_cursor_delta_x"

///|
pub extern "C" fn window_cursor_delta_y() -> Double = "lunarrender_window_cursor_delta_y"

///|
pub extern "C" fn gl_init() -> Bool = "lunarrender_gl_init"

///|
#borrow(vertices)
pub extern "C" fn gl_upload_mesh(
  vertices : FixedArray[Float],
  vertex_count : Int,
) -> Bool = "lunarrender_gl_upload_mesh"

///|
#borrow(view_projection)
pub extern "C" fn gl_draw_mesh(
  view_projection : FixedArray[Float],
  vertex_count : Int,
) -> Unit = "lunarrender_gl_draw_mesh"

///|
pub extern "C" fn gl_clear() -> Unit = "lunarrender_gl_clear"

///|
pub extern "C" fn gl_destroy_renderer() -> Unit = "lunarrender_gl_destroy_renderer"

///|
#borrow(vertices)
pub extern "C" fn gl_upload_chunk_mesh(
  chunk_x : Int,
  chunk_z : Int,
  vertices : FixedArray[Float],
  vertex_count : Int,
  opaque_vertex_count : Int,
  cutout_vertex_count : Int,
  translucent_vertex_count : Int,
) -> Bool = "lunarrender_gl_upload_chunk_mesh"

///|
pub extern "C" fn gl_remove_chunk_mesh(chunk_x : Int, chunk_z : Int) -> Unit =
  "lunarrender_gl_remove_chunk_mesh"

///|
#borrow(view_projection)
pub extern "C" fn gl_draw_chunk_meshes(
  view_projection : FixedArray[Float],
) -> Unit = "lunarrender_gl_draw_chunk_meshes"

///|
#borrow(vertices)
pub extern "C" fn gl_upload_player_mesh(
  vertices : FixedArray[Float],
  vertex_count : Int,
) -> Bool = "lunarrender_gl_upload_player_mesh"

///|
#borrow(view_projection)
pub extern "C" fn gl_draw_player_mesh(
  view_projection : FixedArray[Float],
) -> Unit = "lunarrender_gl_draw_player_mesh"

///|
#borrow(rgba)
pub extern "C" fn gl_replace_player_texture(
  width : Int,
  height : Int,
  rgba : Bytes,
) -> Bool = "lunarrender_gl_replace_player_texture"

///|
pub extern "C" fn gl_draw_hotbar(selected_slot : Int, width : Int, height : Int) -> Unit =
  "lunarrender_gl_draw_hotbar"

///|
#borrow(view_projection)
pub extern "C" fn gl_draw_block_outline(
  view_projection : FixedArray[Float],
  block_x : Int,
  block_y : Int,
  block_z : Int,
) -> Unit = "lunarrender_gl_draw_block_outline"

///|
#borrow(line, status)
pub extern "C" fn gl_draw_command_overlay(
  open : Bool,
  line : String,
  status : String,
  width : Int,
  height : Int,
) -> Unit = "lunarrender_gl_draw_command_overlay"

///|
#borrow(path)
pub extern "C" fn file_read(path : String) -> Bytes = "lunarrender_file_read"

///|
#borrow(path)
pub extern "C" fn file_exists(path : String) -> Bool = "lunarrender_file_exists"

///|
#borrow(path)
pub extern "C" fn image_decode_rgba(path : String) -> Bytes = "lunarrender_image_decode_rgba"

///|
#borrow(rgba)
pub extern "C" fn gl_replace_texture_atlas(
  width : Int,
  height : Int,
  rgba : Bytes,
) -> Bool = "lunarrender_gl_replace_texture_atlas"

///|
#borrow(path, bytes)
pub extern "C" fn save_write_temp(path : String, bytes : Bytes) -> Bool =
  "lunarrender_save_write_temp"

///|
#borrow(temp_path, target_path)
pub extern "C" fn save_replace_temp(temp_path : String, target_path : String) -> Bool =
  "lunarrender_save_replace_temp"

///|
#borrow(path)
pub extern "C" fn save_ensure_directory(path : String) -> Bool =
  "lunarrender_save_ensure_directory"
