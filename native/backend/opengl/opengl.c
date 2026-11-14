#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "glad/gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "resource_table.h"

#define LUNARRENDER_OPENGL_MAX_MESHES 256
#define LUNARRENDER_OPENGL_MAX_TEXTURES 256
#define LUNARRENDER_OPENGL_MAX_MATERIALS 256

typedef struct {
  GLuint vao;
  GLuint vbo;
  int vertex_count;
  int material_key;
} LunarrenderOpenGLMesh;

typedef struct {
  GLuint texture;
} LunarrenderOpenGLTexture;

typedef struct {
  int texture_key;
  float tint[4];
  int alpha_mode;
} LunarrenderOpenGLMaterial;

static bool lunarrender_opengl_initialized = false;
static GLuint lunarrender_opengl_program = 0;
static GLint lunarrender_opengl_matrix_location = -1;
static GLint lunarrender_opengl_texture_location = -1;
static GLint lunarrender_opengl_has_texture_location = -1;
static GLint lunarrender_opengl_tint_location = -1;
static LunarrenderOpenGLMesh
    lunarrender_opengl_meshes[LUNARRENDER_OPENGL_MAX_MESHES];
static LunarrenderOpenGLTexture
    lunarrender_opengl_textures[LUNARRENDER_OPENGL_MAX_TEXTURES];
static LunarrenderOpenGLMaterial
    lunarrender_opengl_materials[LUNARRENDER_OPENGL_MAX_MATERIALS];
static LunarrenderResourceTable lunarrender_opengl_mesh_table;
static LunarrenderResourceTable lunarrender_opengl_texture_table;
static LunarrenderResourceTable lunarrender_opengl_material_table;

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
    "uniform sampler2D u_texture;\n"
    "uniform bool u_has_texture;\n"
    "uniform vec4 u_tint;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  vec4 base = u_has_texture\n"
    "      ? texture(u_texture, v_uv)\n"
    "      : vec4(v_shade, v_shade, v_shade, 1.0);\n"
    "  color = base * u_tint;\n"
    "  if (color.a < 0.03) discard;\n"
    "}\n";

static GLuint lunarrender_opengl_compile_shader(
    GLenum type,
    const char *source) {
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    fprintf(stderr, "LunarRender OpenGL shader compile failed: create\n");
    return 0;
  }
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

static void lunarrender_opengl_clear_errors(void) {
  while (glGetError() != GL_NO_ERROR) {
  }
}

static int lunarrender_opengl_find_mesh(int key) {
  return lr_table_find(&lunarrender_opengl_mesh_table, key);
}

static int lunarrender_opengl_find_texture(int key) {
  return lr_table_find(&lunarrender_opengl_texture_table, key);
}

static int lunarrender_opengl_find_material(int key) {
  return lr_table_find(&lunarrender_opengl_material_table, key);
}

/* Drops the GPU objects of a mesh; the resource table entry is removed by the
 * caller so that a failed upload can reuse the same slot. */
static void lunarrender_opengl_release_mesh(LunarrenderOpenGLMesh *mesh) {
  if (mesh->vbo != 0) {
    glDeleteBuffers(1, &mesh->vbo);
    mesh->vbo = 0;
  }
  if (mesh->vao != 0) {
    glDeleteVertexArrays(1, &mesh->vao);
    mesh->vao = 0;
  }
  mesh->vertex_count = 0;
  mesh->material_key = -1;
}

static void lunarrender_opengl_release_texture(
    LunarrenderOpenGLTexture *texture) {
  if (texture->texture != 0) {
    glDeleteTextures(1, &texture->texture);
    texture->texture = 0;
  }
}

static void lunarrender_opengl_release_material(
    LunarrenderOpenGLMaterial *material) {
  material->texture_key = -1;
  material->tint[0] = 1.0f;
  material->tint[1] = 1.0f;
  material->tint[2] = 1.0f;
  material->tint[3] = 1.0f;
  material->alpha_mode = 0;
}

static void lunarrender_opengl_reset_slots(void) {
  lr_table_init(&lunarrender_opengl_mesh_table, LUNARRENDER_OPENGL_MAX_MESHES);
  lr_table_init(
      &lunarrender_opengl_texture_table,
      LUNARRENDER_OPENGL_MAX_TEXTURES);
  lr_table_init(
      &lunarrender_opengl_material_table,
      LUNARRENDER_OPENGL_MAX_MATERIALS);
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MESHES; i++) {
    lunarrender_opengl_meshes[i].vao = 0;
    lunarrender_opengl_meshes[i].vbo = 0;
    lunarrender_opengl_meshes[i].vertex_count = 0;
    lunarrender_opengl_meshes[i].material_key = -1;
  }
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_TEXTURES; i++) {
    lunarrender_opengl_textures[i].texture = 0;
  }
  for (int i = 0; i < LUNARRENDER_OPENGL_MAX_MATERIALS; i++) {
    lunarrender_opengl_release_material(&lunarrender_opengl_materials[i]);
  }
}

int32_t lunarrender_opengl_init(void) {
  if (lunarrender_opengl_initialized) {
    return 1;
  }
  lunarrender_opengl_reset_slots();
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
  lunarrender_opengl_matrix_location = glGetUniformLocation(
      lunarrender_opengl_program, "u_view_projection");
  lunarrender_opengl_texture_location = glGetUniformLocation(
      lunarrender_opengl_program, "u_texture");
  lunarrender_opengl_has_texture_location = glGetUniformLocation(
      lunarrender_opengl_program, "u_has_texture");
  lunarrender_opengl_tint_location = glGetUniformLocation(
      lunarrender_opengl_program, "u_tint");
  glUseProgram(lunarrender_opengl_program);
  if (lunarrender_opengl_texture_location >= 0) {
    glUniform1i(lunarrender_opengl_texture_location, 0);
  }
  glUseProgram(0);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.08f, 0.12f, 0.20f, 1.0f);
  lunarrender_opengl_initialized = true;
  fprintf(stderr, "LunarRender OpenGL backend: %s\n", version);
  return 1;
}

int32_t lunarrender_opengl_upload_texture(
    int key,
    int width,
    int height,
    const uint8_t *rgba) {
  if (!lunarrender_opengl_initialized || width <= 0 || height <= 0 ||
      rgba == NULL) {
    fprintf(stderr, "LunarRender OpenGL texture upload rejected: invalid data\n");
    return 0;
  }
  int slot = lr_table_acquire(&lunarrender_opengl_texture_table, key);
  if (slot < 0) {
    fprintf(stderr, "LunarRender OpenGL texture upload rejected: slots full\n");
    return 0;
  }
  LunarrenderOpenGLTexture *texture = &lunarrender_opengl_textures[slot];
  lunarrender_opengl_release_texture(texture);
  lunarrender_opengl_clear_errors();
  glGenTextures(1, &texture->texture);
  if (texture->texture == 0) {
    lr_table_remove(&lunarrender_opengl_texture_table, slot);
    fprintf(stderr, "LunarRender OpenGL texture upload failed: texture creation\n");
    return 0;
  }
  glBindTexture(GL_TEXTURE_2D, texture->texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      width,
      height,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      rgba);
  glBindTexture(GL_TEXTURE_2D, 0);
  if (glGetError() != GL_NO_ERROR) {
    lunarrender_opengl_release_texture(texture);
    lr_table_remove(&lunarrender_opengl_texture_table, slot);
    fprintf(stderr, "LunarRender OpenGL texture upload failed: GPU error\n");
    return 0;
  }
  fprintf(
      stderr,
      "LunarRender OpenGL texture upload: key=%d size=%dx%d\n",
      key,
      width,
      height);
  return 1;
}

void lunarrender_opengl_remove_texture(int key) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  int slot = lunarrender_opengl_find_texture(key);
  if (slot >= 0) {
    lunarrender_opengl_release_texture(&lunarrender_opengl_textures[slot]);
    lr_table_remove(&lunarrender_opengl_texture_table, slot);
  }
}

int32_t lunarrender_opengl_upload_material(
    int key,
    int texture_key,
    const float *tint,
    int alpha_mode) {
  if (!lunarrender_opengl_initialized || tint == NULL ||
      (alpha_mode != 0 && alpha_mode != 1)) {
    fprintf(stderr, "LunarRender OpenGL material upload rejected: invalid data\n");
    return 0;
  }
  if (texture_key >= 0 && lunarrender_opengl_find_texture(texture_key) < 0) {
    fprintf(
        stderr,
        "LunarRender OpenGL material upload rejected: missing texture key=%d\n",
        texture_key);
    return 0;
  }
  int slot = lr_table_acquire(&lunarrender_opengl_material_table, key);
  if (slot < 0) {
    fprintf(stderr, "LunarRender OpenGL material upload rejected: slots full\n");
    return 0;
  }
  LunarrenderOpenGLMaterial *material = &lunarrender_opengl_materials[slot];
  lunarrender_opengl_release_material(material);
  material->texture_key = texture_key;
  material->tint[0] = tint[0];
  material->tint[1] = tint[1];
  material->tint[2] = tint[2];
  material->tint[3] = tint[3];
  material->alpha_mode = alpha_mode;
  fprintf(
      stderr,
      "LunarRender OpenGL material upload: key=%d texture=%d alpha=%d\n",
      key,
      texture_key,
      alpha_mode);
  return 1;
}

void lunarrender_opengl_remove_material(int key) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  int slot = lunarrender_opengl_find_material(key);
  if (slot >= 0) {
    lunarrender_opengl_release_material(&lunarrender_opengl_materials[slot]);
    lr_table_remove(&lunarrender_opengl_material_table, slot);
  }
}

int32_t lunarrender_opengl_upload_mesh(
    int key,
    int material_key,
    const float *vertices,
    int vertex_count,
    int floats_per_vertex) {
  if (!lunarrender_opengl_initialized || vertex_count < 0 ||
      floats_per_vertex != 6 || (vertex_count > 0 && vertices == NULL)) {
    fprintf(stderr, "LunarRender OpenGL mesh upload rejected: invalid data\n");
    return 0;
  }
  if (material_key >= 0 && lunarrender_opengl_find_material(material_key) < 0) {
    fprintf(
        stderr,
        "LunarRender OpenGL mesh upload rejected: missing material key=%d\n",
        material_key);
    return 0;
  }
  int slot = lr_table_acquire(&lunarrender_opengl_mesh_table, key);
  if (slot < 0) {
    fprintf(stderr, "LunarRender OpenGL mesh upload rejected: slots full\n");
    return 0;
  }
  LunarrenderOpenGLMesh *mesh = &lunarrender_opengl_meshes[slot];
  bool reused = mesh->vao != 0 && mesh->vbo != 0;
  lunarrender_opengl_clear_errors();
  if (!reused) {
    lunarrender_opengl_release_mesh(mesh);
    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    if (mesh->vao == 0 || mesh->vbo == 0) {
      lunarrender_opengl_release_mesh(mesh);
      lr_table_remove(&lunarrender_opengl_mesh_table, slot);
      fprintf(stderr, "LunarRender OpenGL mesh upload failed: buffer creation\n");
      return 0;
    }
  }
  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)vertex_count * floats_per_vertex * (int)sizeof(float),
      vertices,
      GL_STATIC_DRAW);
  if (!reused) {
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
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  if (glGetError() != GL_NO_ERROR) {
    lunarrender_opengl_release_mesh(mesh);
    lr_table_remove(&lunarrender_opengl_mesh_table, slot);
    fprintf(stderr, "LunarRender OpenGL mesh upload failed: GPU error\n");
    return 0;
  }
  mesh->vertex_count = vertex_count;
  mesh->material_key = material_key;
  fprintf(
      stderr,
      "LunarRender OpenGL mesh upload: key=%d vertices=%d material=%d\n",
      key,
      vertex_count,
      material_key);
  return 1;
}

void lunarrender_opengl_remove_mesh(int key) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  int slot = lunarrender_opengl_find_mesh(key);
  if (slot >= 0) {
    lunarrender_opengl_release_mesh(&lunarrender_opengl_meshes[slot]);
    lr_table_remove(&lunarrender_opengl_mesh_table, slot);
  }
}

void lunarrender_opengl_draw_frame(
    const float *view_projection,
    int viewport_width,
    int viewport_height) {
  if (!lunarrender_opengl_initialized || view_projection == NULL ||
      viewport_width <= 0 || viewport_height <= 0) {
    return;
  }
  glViewport(0, 0, viewport_width, viewport_height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(lunarrender_opengl_program);
  if (lunarrender_opengl_matrix_location >= 0) {
    glUniformMatrix4fv(
        lunarrender_opengl_matrix_location,
        1,
        GL_FALSE,
        view_projection);
  }
  /* Dense iteration: active[] holds exactly the live meshes, so a frame costs
   * O(active meshes) instead of O(slot capacity). Bind state changes are also
   * deduplicated across the batch. */
  GLuint last_texture = 0;
  int last_has_texture = -1;
  int blend_enabled = 0;
  float last_tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  int last_tint_valid = 0;
  glActiveTexture(GL_TEXTURE0);
  for (int position = 0;
       position < lunarrender_opengl_mesh_table.active_count;
       position++) {
    int mesh_slot = lunarrender_opengl_mesh_table.active[position];
    LunarrenderOpenGLMesh *mesh = &lunarrender_opengl_meshes[mesh_slot];
    if (mesh->vertex_count == 0) {
      continue;
    }
    int material_slot = lunarrender_opengl_find_material(mesh->material_key);
    int has_texture = 0;
    int alpha_mode = 0;
    float tint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLuint texture = 0;
    if (material_slot >= 0) {
      LunarrenderOpenGLMaterial *material =
          &lunarrender_opengl_materials[material_slot];
      tint[0] = material->tint[0];
      tint[1] = material->tint[1];
      tint[2] = material->tint[2];
      tint[3] = material->tint[3];
      alpha_mode = material->alpha_mode;
      int texture_slot = lunarrender_opengl_find_texture(material->texture_key);
      if (texture_slot >= 0) {
        has_texture = 1;
        texture = lunarrender_opengl_textures[texture_slot].texture;
      }
    }
    if (texture != last_texture) {
      glBindTexture(GL_TEXTURE_2D, texture);
      last_texture = texture;
    }
    if (has_texture != last_has_texture) {
      if (lunarrender_opengl_has_texture_location >= 0) {
        glUniform1i(lunarrender_opengl_has_texture_location, has_texture);
      }
      last_has_texture = has_texture;
    }
    if (!last_tint_valid || tint[0] != last_tint[0] ||
        tint[1] != last_tint[1] || tint[2] != last_tint[2] ||
        tint[3] != last_tint[3]) {
      if (lunarrender_opengl_tint_location >= 0) {
        glUniform4fv(lunarrender_opengl_tint_location, 1, tint);
      }
      last_tint[0] = tint[0];
      last_tint[1] = tint[1];
      last_tint[2] = tint[2];
      last_tint[3] = tint[3];
      last_tint_valid = 1;
    }
    if (alpha_mode == 1) {
      if (!blend_enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        blend_enabled = 1;
      }
    } else if (blend_enabled) {
      glDisable(GL_BLEND);
      blend_enabled = 0;
    }
    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
  }
  glDisable(GL_BLEND);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}

void lunarrender_opengl_shutdown(void) {
  if (!lunarrender_opengl_initialized) {
    return;
  }
  for (int position = 0;
       position < lunarrender_opengl_mesh_table.active_count;
       position++) {
    lunarrender_opengl_release_mesh(
        &lunarrender_opengl_meshes[lunarrender_opengl_mesh_table.active[position]]);
  }
  for (int position = 0;
       position < lunarrender_opengl_texture_table.active_count;
       position++) {
    lunarrender_opengl_release_texture(
        &lunarrender_opengl_textures
             [lunarrender_opengl_texture_table.active[position]]);
  }
  for (int position = 0;
       position < lunarrender_opengl_material_table.active_count;
       position++) {
    lunarrender_opengl_release_material(
        &lunarrender_opengl_materials
             [lunarrender_opengl_material_table.active[position]]);
  }
  lunarrender_opengl_reset_slots();
  if (lunarrender_opengl_program != 0) {
    glDeleteProgram(lunarrender_opengl_program);
    lunarrender_opengl_program = 0;
  }
  lunarrender_opengl_matrix_location = -1;
  lunarrender_opengl_texture_location = -1;
  lunarrender_opengl_has_texture_location = -1;
  lunarrender_opengl_tint_location = -1;
  lunarrender_opengl_initialized = false;
  fprintf(stderr, "LunarRender OpenGL backend: destroyed\n");
}
