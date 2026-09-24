#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static GLFWwindow *lunarrender_platform_window = NULL;
static double lunarrender_platform_cursor_x = 0.0;
static double lunarrender_platform_cursor_y = 0.0;
static bool lunarrender_platform_cursor_initialized = false;
static double lunarrender_platform_delta_x = 0.0;
static double lunarrender_platform_delta_y = 0.0;

int32_t lunarrender_platform_window_create(int width, int height) {
  if (lunarrender_platform_window != NULL) {
    return 1;
  }
  if (glfwInit() == GLFW_FALSE) {
    return 0;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  lunarrender_platform_window =
      glfwCreateWindow(width, height, "LunarRender", NULL, NULL);
  if (lunarrender_platform_window == NULL) {
    glfwTerminate();
    return 0;
  }
  glfwMakeContextCurrent(lunarrender_platform_window);
  glfwSwapInterval(1);
  glfwGetCursorPos(
      lunarrender_platform_window,
      &lunarrender_platform_cursor_x,
      &lunarrender_platform_cursor_y);
  lunarrender_platform_cursor_initialized = true;
  lunarrender_platform_delta_x = 0.0;
  lunarrender_platform_delta_y = 0.0;
  fprintf(stderr, "LunarRender platform window: %dx%d\n", width, height);
  return 1;
}

int32_t lunarrender_platform_window_should_close(void) {
  return lunarrender_platform_window == NULL ||
      glfwWindowShouldClose(lunarrender_platform_window) != 0;
}

void lunarrender_platform_window_poll_events(void) {
  if (lunarrender_platform_window == NULL) {
    return;
  }
  glfwPollEvents();
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(lunarrender_platform_window, &x, &y);
  if (!lunarrender_platform_cursor_initialized) {
    lunarrender_platform_cursor_x = x;
    lunarrender_platform_cursor_y = y;
    lunarrender_platform_cursor_initialized = true;
    lunarrender_platform_delta_x = 0.0;
    lunarrender_platform_delta_y = 0.0;
    return;
  }
  lunarrender_platform_delta_x = x - lunarrender_platform_cursor_x;
  lunarrender_platform_delta_y = y - lunarrender_platform_cursor_y;
  lunarrender_platform_cursor_x = x;
  lunarrender_platform_cursor_y = y;
}

void lunarrender_platform_window_swap_buffers(void) {
  if (lunarrender_platform_window != NULL) {
    glfwSwapBuffers(lunarrender_platform_window);
  }
}

void lunarrender_platform_window_destroy(void) {
  if (lunarrender_platform_window != NULL) {
    glfwDestroyWindow(lunarrender_platform_window);
    lunarrender_platform_window = NULL;
    glfwTerminate();
  }
  lunarrender_platform_delta_x = 0.0;
  lunarrender_platform_delta_y = 0.0;
  lunarrender_platform_cursor_initialized = false;
  fprintf(stderr, "LunarRender platform window: destroyed\n");
}

int32_t lunarrender_platform_window_framebuffer_width(void) {
  if (lunarrender_platform_window == NULL) {
    return 0;
  }
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(lunarrender_platform_window, &width, &height);
  return (int32_t)width;
}

int32_t lunarrender_platform_window_framebuffer_height(void) {
  if (lunarrender_platform_window == NULL) {
    return 0;
  }
  int width = 0;
  int height = 0;
  glfwGetFramebufferSize(lunarrender_platform_window, &width, &height);
  return (int32_t)height;
}

double lunarrender_platform_window_time_seconds(void) {
  if (lunarrender_platform_window == NULL) {
    return 0.0;
  }
  return glfwGetTime();
}

int32_t lunarrender_platform_window_key_down(int key) {
  return lunarrender_platform_window != NULL &&
      glfwGetKey(lunarrender_platform_window, key) == GLFW_PRESS;
}

int32_t lunarrender_platform_window_mouse_button_down(int button) {
  return lunarrender_platform_window != NULL &&
      glfwGetMouseButton(lunarrender_platform_window, button) == GLFW_PRESS;
}

double lunarrender_platform_window_cursor_delta_x(void) {
  double value = lunarrender_platform_delta_x;
  lunarrender_platform_delta_x = 0.0;
  return value;
}

double lunarrender_platform_window_cursor_delta_y(void) {
  double value = lunarrender_platform_delta_y;
  lunarrender_platform_delta_y = 0.0;
  return value;
}
