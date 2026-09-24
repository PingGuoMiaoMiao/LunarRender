#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "glad/gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define LUNARRENDER_OPENGL_MAX_MESHES 256

typedef struct {
  bool active;
  int key;
  GLuint vao;
  GLuint vbo;
  int vertex_count;
} LunarrenderOpenGLMesh;

static bool lunarrender_opengl_initialized = false;
static GLuint lunarrender_opengl_program = 0;
static LunarrenderOpenGLMesh
    lunarrender_opengl_meshes[LUNARRENDER_OPENGL_MAX_MESHES];

static const char *lunarrender_opengl_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "layout(location = 2) in float a_shade;\n"
    "uniform mat4 u_view_projection;\n"
    "out vec2 v_uv;\n"
    "out float v_shade;\n"
    "void main() {\n"
    "  gl_Position = u_view_projection * vec4(a_position, 1.0);\n"
    "  v_uv = a_uv;\n"
    "  v_shade = a_shade;\n"
    "}\n";

static const char *lunarrender_opengl_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "in float v_shade;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  vec3 base = vec3(0.25 + v_uv.x * 0.45, 0.55 + v_uv.y * 0.25, 0.9);\n"
    "  color = vec4(base * v_shade, 1.0);\n"
    "}\n";

static GLuint lunarrender_opengl_compile_shader(
    GLenum type,
    const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint compiled = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE) {
    char log[2048] = {0};
    GLsizei length = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(log) - 1, &length, log);
    fprintf(stderr, "LunarRender OpenGL shader compile failed: %s\n", log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static int lunarrender_opengl_find_mesh(int key) {
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MESHES; i++) {
    if (lunarrender_opengl_meshes[i].active &&
        lunarrender_opengl_meshes[i].key == key) {
      return i;
    }
  }
  return -1;
}

static int lunarrender_opengl_find_free_mesh(void) {
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MESHES; i++) {
    if (!lunarrender_opengl_meshes[i].active) {
      return i;
    }
  }
  return -1;
}

static void lunarrender_opengl_release_mesh(LunarrenderOpenGLMesh *mesh) {
  if (mesh->vbo != 0) {
    glDeleteBuffers(1, &mesh->vbo);
    mesh->vbo = 0;
  }
  if (mesh->vao != 0) {
    glDeleteVertexArrays(1, &mesh->vao);
    mesh->vao = 0;
  }
  mesh->active = false;
  mesh->vertex_count = 0;
}

int32_t lunarrender_opengl_init(void) {
  if (lunarrender_opengl_initialized) {
    return 1;
  }
  if (gladLoadGL((GLADloadfunc)glfwGetProcAddress) == 0) {
    fprintf(stderr, "LunarRender OpenGL init failed: GLAD load failed\n");
    return 0;
  }
  const GLubyte *version = glGetString(GL_VERSION);
  if (version == NULL) {
    fprintf(stderr, "LunarRender OpenGL init failed: no context\n");
    return 0;
  }
  GLuint vertex = lunarrender_opengl_compile_shader(
      GL_VERTEX_SHADER, lunarrender_opengl_vertex_shader);
  GLuint fragment = lunarrender_opengl_compile_shader(
      GL_FRAGMENT_SHADER, lunarrender_opengl_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    fprintf(stderr, "LunarRender OpenGL init failed: shader stage\n");
    return 0;
  }
  lunarrender_opengl_program = glCreateProgram();
  glAttachShader(lunarrender_opengl_program, vertex);
  glAttachShader(lunarrender_opengl_program, fragment);
  glLinkProgram(lunarrender_opengl_program);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  GLint linked = GL_FALSE;
  glGetProgramiv(lunarrender_opengl_program, GL_LINK_STATUS, &linked);
  if (linked == GL_FALSE) {
    char log[2048] = {0};
    GLsizei length = 0;
    glGetProgramInfoLog(
        lunarrender_opengl_program,
        (GLsizei)sizeof(log) - 1,
        &length,
        log);
    fprintf(stderr, "LunarRender OpenGL program link failed: %s\n", log);
    glDeleteProgram(lunarrender_opengl_program);
    lunarrender_opengl_program = 0;
    return 0;
  }
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.08f, 0.12f, 0.20f, 1.0f);
  lunarrender_opengl_initialized = true;
  fprintf(stderr, "LunarRender OpenGL backend: %s\n", version);
  return 1;
}

int32_t lunarrender_opengl_upload_mesh(
    int key,
    const float *vertices,
    int vertex_count,
    int floats_per_vertex) {
  if (!lunarrender_opengl_initialized || vertex_count < 0 ||
      floats_per_vertex != 6 || (vertex_count > 0 && vertices == NULL)) {
    fprintf(stderr, "LunarRender OpenGL mesh upload rejected: invalid data\n");
    return 0;
  }
  int index = lunarrender_opengl_find_mesh(key);
  if (index < 0) {
    index = lunarrender_opengl_find_free_mesh();
  }
  if (index < 0) {
    fprintf(stderr, "LunarRender OpenGL mesh upload rejected: slots full\n");
    return 0;
  }
  lunarrender_opengl_release_mesh(&lunarrender_opengl_meshes[index]);
  LunarrenderOpenGLMesh *mesh = &lunarrender_opengl_meshes[index];
  glGenVertexArrays(1, &mesh->vao);
  glGenBuffers(1, &mesh->vbo);
  if (mesh->vao == 0 || mesh->vbo == 0) {
    lunarrender_opengl_release_mesh(mesh);
    fprintf(stderr, "LunarRender OpenGL mesh upload failed: buffer creation\n");
    return 0;
  }
  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)vertex_count * floats_per_vertex * (int)sizeof(float),
      vertices,
      GL_STATIC_DRAW);
  GLsizei stride = (GLsizei)floats_per_vertex * (GLsizei)sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
      1,
      2,
      GL_FLOAT,
      GL_FALSE,
      stride,
      (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
      2,
      1,
      GL_FLOAT,
      GL_FALSE,
      stride,
      (void *)(5 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  if (glGetError() != GL_NO_ERROR) {
    lunarrender_opengl_release_mesh(mesh);
    fprintf(stderr, "LunarRender OpenGL mesh upload failed: GPU error\n");
    return 0;
  }
  mesh->active = true;
  mesh->key = key;
  mesh->vertex_count = vertex_count;
  fprintf(
      stderr,
      "LunarRender OpenGL mesh upload: key=%d vertices=%d\n",
      key,
      vertex_count);
  return 1;
}

void lunarrender_opengl_remove_mesh(int key) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  int index = lunarrender_opengl_find_mesh(key);
  if (index >= 0) {
    lunarrender_opengl_release_mesh(&lunarrender_opengl_meshes[index]);
  }
}

void lunarrender_opengl_draw_frame(const float *view_projection) {
  if (!lunarrender_opengl_initialized || view_projection == NULL) {
    return;
  }
  glViewport(0, 0, 960, 540);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(lunarrender_opengl_program);
  GLint matrix_location = glGetUniformLocation(
      lunarrender_opengl_program,
      "u_view_projection");
  glUniformMatrix4fv(matrix_location, 1, GL_FALSE, view_projection);
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MESHES; i++) {
    LunarrenderOpenGLMesh *mesh = &lunarrender_opengl_meshes[i];
    if (!mesh->active || mesh->vertex_count == 0) {
      continue;
    }
    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
  }
  glBindVertexArray(0);
  glUseProgram(0);
}

void lunarrender_opengl_shutdown(void) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MESHES; i++) {
    lunarrender_opengl_release_mesh(&lunarrender_opengl_meshes[i]);
  }
  if (lunarrender_opengl_program != 0) {
    glDeleteProgram(lunarrender_opengl_program);
    lunarrender_opengl_program = 0;
  }
  lunarrender_opengl_initialized = false;
  fprintf(stderr, "LunarRender OpenGL backend: destroyed\n");
}
