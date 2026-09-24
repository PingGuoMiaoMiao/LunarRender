///|
pub extern "C" fn window_create(width : Int, height : Int) -> Bool =
  "lunarrender_platform_window_create"

///|
pub extern "C" fn window_should_close() -> Bool =
  "lunarrender_platform_window_should_close"

///|
pub extern "C" fn window_poll_events() -> Unit =
  "lunarrender_platform_window_poll_events"

///|
pub extern "C" fn window_swap_buffers() -> Unit =
  "lunarrender_platform_window_swap_buffers"

///|
pub extern "C" fn window_destroy() -> Unit =
  "lunarrender_platform_window_destroy"

///|
pub extern "C" fn window_framebuffer_width() -> Int =
  "lunarrender_platform_window_framebuffer_width"

///|
pub extern "C" fn window_framebuffer_height() -> Int =
  "lunarrender_platform_window_framebuffer_height"

///|
pub extern "C" fn window_time_seconds() -> Double =
  "lunarrender_platform_window_time_seconds"

///|
pub extern "C" fn window_key_down(key : Int) -> Bool =
  "lunarrender_platform_window_key_down"

///|
pub extern "C" fn window_mouse_button_down(button : Int) -> Bool =
  "lunarrender_platform_window_mouse_button_down"

///|
pub extern "C" fn window_cursor_delta_x() -> Double =
  "lunarrender_platform_window_cursor_delta_x"

///|
pub extern "C" fn window_cursor_delta_y() -> Double =
  "lunarrender_platform_window_cursor_delta_y"

///|
#borrow(path)
pub extern "C" fn file_read(path : String) -> Bytes = "lunarrender_file_read"

///|
#borrow(path)
pub extern "C" fn file_exists(path : String) -> Bool = "lunarrender_file_exists"

///|
#borrow(path)
pub extern "C" fn image_decode_rgba(path : String) -> Bytes =
  "lunarrender_image_decode_rgba"
