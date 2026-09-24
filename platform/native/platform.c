#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "moonbit.h"
#include "glad/gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static GLFWwindow *lunarrender_window = NULL;
static GLuint lunarrender_program = 0;
static GLuint lunarrender_vao = 0;
static GLuint lunarrender_vbo = 0;
static GLuint lunarrender_texture = 0;
static GLuint lunarrender_sky_program = 0;
static GLuint lunarrender_sky_vao = 0;
static GLuint lunarrender_sky_vbo = 0;
static GLuint lunarrender_ui_program = 0;
static GLuint lunarrender_ui_vao = 0;
static GLuint lunarrender_ui_vbo = 0;
static GLuint lunarrender_ui_texture_program = 0;
static GLuint lunarrender_ui_texture_vao = 0;
static GLuint lunarrender_ui_texture_vbo = 0;
static GLuint lunarrender_outline_program = 0;
static GLuint lunarrender_outline_vao = 0;
static GLuint lunarrender_outline_vbo = 0;
static GLuint lunarrender_player_program = 0;
static GLuint lunarrender_player_vao = 0;
static GLuint lunarrender_player_vbo = 0;
static GLuint lunarrender_player_texture = 0;
static int lunarrender_player_vertex_count = 0;
#ifdef LUNARRENDER_ENABLE_MMD
static GLuint lunarrender_mmd_program = 0;
static GLuint lunarrender_mmd_vao = 0;
static GLuint lunarrender_mmd_vbo = 0;
static GLuint lunarrender_mmd_texture = 0;
static bool lunarrender_mmd_draw_reported = false;
static bool lunarrender_mmd_gl_error_reported = false;

typedef struct {
  int first_vertex;
  int vertex_count;
  int bone_count;
  int palette_offset;
  int hand;
} lunarrender_mmd_animated_mesh_t;
#endif
static double lunarrender_cursor_x = 0.0;
static double lunarrender_cursor_y = 0.0;
static bool lunarrender_cursor_initialized = false;
static int lunarrender_cursor_warmup_frames = 0;
static double lunarrender_delta_x = 0.0;
static double lunarrender_delta_y = 0.0;

#define LUNARRENDER_MAX_CHUNKS 512

typedef struct {
  bool active;
  int chunk_x;
  int chunk_z;
  GLuint vao;
  GLuint vbo;
  int vertex_count;
  int opaque_vertex_count;
  int cutout_vertex_count;
  int translucent_vertex_count;
} LunarrenderChunkGpu;

static LunarrenderChunkGpu lunarrender_chunks[LUNARRENDER_MAX_CHUNKS];
static bool lunarrender_chunk_upload_reported = false;
static bool lunarrender_player_upload_reported = false;

extern const unsigned char lunarrender_texture_atlas_rgba[];
extern const int lunarrender_texture_atlas_width;
extern const int lunarrender_texture_atlas_height;
#ifdef LUNARRENDER_ENABLE_MMD
extern const float lunarrender_mmd_animated_vertices[];
extern const int lunarrender_mmd_animated_vertex_count;
extern const int lunarrender_mmd_animated_mesh_count;
extern const int lunarrender_mmd_animated_frame_count;
extern const int lunarrender_mmd_animated_frame_rate;
extern const int lunarrender_mmd_animated_palette_stride;
extern const lunarrender_mmd_animated_mesh_t lunarrender_mmd_animated_meshes[];
extern const float lunarrender_mmd_animated_bone_matrices[];
extern const float lunarrender_mmd_animated_model_matrix[];
extern const unsigned char lunarrender_mmd_texture_atlas_rgba[];
extern const int lunarrender_mmd_texture_atlas_width;
extern const int lunarrender_mmd_texture_atlas_height;
#endif

static const char *lunarrender_vertex_shader =
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

static const char *lunarrender_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "in float v_shade;\n"
    "uniform sampler2D u_texture;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  vec4 texel = texture(u_texture, v_uv);\n"
    "  if (texel.a < 0.03) discard;\n"
    "  vec3 lit = texel.rgb * v_shade;\n"
    "  float fog = smoothstep(0.72, 0.99, gl_FragCoord.z);\n"
    "  vec3 sky = vec3(0.53, 0.75, 0.92);\n"
    "  color = vec4(mix(lit, sky, fog), texel.a);\n"
    "}\n";

static const char *lunarrender_sky_vertex_shader =
    "#version 330 core\n"
    "const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  vec2 position = positions[gl_VertexID];\n"
    "  v_uv = position * 0.5 + 0.5;\n"
    "  gl_Position = vec4(position, 1.0, 1.0);\n"
    "}\n";

static const char *lunarrender_sky_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "out vec4 color;\n"
    "float cloud_rect(vec2 point, vec2 center, vec2 half_size) {\n"
    "  vec2 delta = abs(point - center) - half_size;\n"
    "  float edge = max(delta.x, delta.y);\n"
    "  return 1.0 - smoothstep(-0.006, 0.006, edge);\n"
    "}\n"
    "void main() {\n"
    "  float height = clamp(v_uv.y, 0.0, 1.0);\n"
    "  vec3 horizon = vec3(0.78, 0.88, 0.96);\n"
    "  vec3 zenith = vec3(0.25, 0.58, 0.86);\n"
    "  vec3 sky = mix(horizon, zenith, pow(height, 0.72));\n"
    "  float clouds = 0.0;\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.12, 0.78), vec2(0.11, 0.018)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.06, 0.805), vec2(0.035, 0.022)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.18, 0.81), vec2(0.042, 0.028)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.42, 0.73), vec2(0.14, 0.018)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.36, 0.755), vec2(0.052, 0.026)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.51, 0.765), vec2(0.045, 0.030)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.70, 0.82), vec2(0.13, 0.018)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.65, 0.845), vec2(0.046, 0.028)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.78, 0.842), vec2(0.048, 0.032)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.91, 0.75), vec2(0.12, 0.018)));\n"
    "  clouds = max(clouds, cloud_rect(v_uv, vec2(0.86, 0.775), vec2(0.042, 0.026)));\n"
    "  sky = mix(sky, vec3(0.97, 0.985, 1.0), clouds * 0.92);\n"
    "  color = vec4(sky, 1.0);\n"
    "}\n";

static const char *lunarrender_ui_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_position;\n"
    "layout(location = 1) in vec3 a_color;\n"
    "uniform vec2 u_viewport;\n"
    "out vec3 v_color;\n"
    "void main() {\n"
    "  vec2 ndc = vec2(a_position.x / u_viewport.x * 2.0 - 1.0,\n"
    "    1.0 - a_position.y / u_viewport.y * 2.0);\n"
    "  gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "  v_color = a_color;\n"
    "}\n";

static const char *lunarrender_ui_fragment_shader =
    "#version 330 core\n"
    "in vec3 v_color;\n"
    "out vec4 color;\n"
    "void main() { color = vec4(v_color, 0.94); }\n";

static const char *lunarrender_ui_texture_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec2 a_position;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "uniform vec2 u_viewport;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  vec2 ndc = vec2(a_position.x / u_viewport.x * 2.0 - 1.0,\n"
    "    1.0 - a_position.y / u_viewport.y * 2.0);\n"
    "  gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "  v_uv = a_uv;\n"
    "}\n";

static const char *lunarrender_ui_texture_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_texture;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  color = texture(u_texture, v_uv);\n"
    "  if (color.a < 0.03) discard;\n"
    "}\n";

static const char *lunarrender_outline_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "uniform mat4 u_view_projection;\n"
    "void main() { gl_Position = u_view_projection * vec4(a_position, 1.0); }\n";

static const char *lunarrender_outline_fragment_shader =
    "#version 330 core\n"
    "out vec4 color;\n"
    "void main() { color = vec4(0.015, 0.015, 0.015, 0.88); }\n";

static const char *lunarrender_player_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "uniform mat4 u_view_projection;\n"
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  gl_Position = u_view_projection * vec4(a_position, 1.0);\n"
    "  v_uv = a_uv;\n"
    "}\n";

static const char *lunarrender_player_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "uniform sampler2D u_texture;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  color = texture(u_texture, v_uv);\n"
    "  if (color.a < 0.03) discard;\n"
    "}\n";

#ifdef LUNARRENDER_ENABLE_MMD
static const char *lunarrender_mmd_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec3 a_normal;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "layout(location = 3) in vec4 a_diffuse;\n"
    "layout(location = 4) in float a_toon;\n"
    "layout(location = 5) in vec4 a_bone_indices;\n"
    "layout(location = 6) in vec4 a_bone_weights;\n"
    "uniform mat4 u_view_projection;\n"
    "uniform mat4 u_model_matrix;\n"
    "uniform vec3 u_entity_offset;\n"
    "uniform mat4 u_bones[64];\n"
    "out vec2 v_uv;\n"
    "out vec4 v_diffuse;\n"
    "out vec3 v_normal;\n"
    "void main() {\n"
    "  mat4 skin = a_bone_weights.x * u_bones[int(a_bone_indices.x)] +\n"
    "    a_bone_weights.y * u_bones[int(a_bone_indices.y)] +\n"
    "    a_bone_weights.z * u_bones[int(a_bone_indices.z)] +\n"
    "    a_bone_weights.w * u_bones[int(a_bone_indices.w)];\n"
    "  mat4 world_matrix = u_model_matrix * skin;\n"
    "  vec4 world_position = world_matrix * vec4(a_position, 1.0);\n"
    "  world_position.xyz += u_entity_offset;\n"
    "  gl_Position = u_view_projection * world_position;\n"
    "  v_uv = a_uv;\n"
    "  v_diffuse = a_diffuse;\n"
    "  v_normal = normalize(mat3(world_matrix) * a_normal);\n"
    "}\n";

static const char *lunarrender_mmd_fragment_shader =
    "#version 330 core\n"
    "in vec2 v_uv;\n"
    "in vec4 v_diffuse;\n"
    "in vec3 v_normal;\n"
    "uniform sampler2D u_texture;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "  vec4 texel = texture(u_texture, v_uv);\n"
    "  float light = 0.62 + 0.38 * max(dot(normalize(v_normal), normalize(vec3(-0.45, 0.85, 0.35))), 0.0);\n"
    "  color = vec4(texel.rgb * v_diffuse.rgb * light, texel.a * v_diffuse.a);\n"
    "  if (color.a < 0.03) discard;\n"
    "}\n";
#endif

static GLuint lunarrender_compile_shader(GLenum type, const char *source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    char log[2048];
    GLsizei length = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender shader compile failed: %.*s\n", (int)length, log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

static int32_t lunarrender_prepare_voxel_program(void) {
  if (lunarrender_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_program = glCreateProgram();
  glAttachShader(lunarrender_program, vertex);
  glAttachShader(lunarrender_program, fragment);
  glLinkProgram(lunarrender_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_program);
    lunarrender_program = 0;
    return false;
  }
  return true;
}

static void lunarrender_configure_chunk_vao(LunarrenderChunkGpu *chunk) {
  if (chunk->vao != 0 && chunk->vbo != 0) {
    return;
  }
  glGenVertexArrays(1, &chunk->vao);
  glGenBuffers(1, &chunk->vbo);
  glBindVertexArray(chunk->vao);
  glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)(5 * sizeof(float)));
  glEnableVertexAttribArray(2);
}

static int32_t lunarrender_prepare_ui_program(void) {
  if (lunarrender_ui_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_ui_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_ui_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_ui_program = glCreateProgram();
  glAttachShader(lunarrender_ui_program, vertex);
  glAttachShader(lunarrender_ui_program, fragment);
  glLinkProgram(lunarrender_ui_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_ui_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_ui_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender hotbar shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_ui_program);
    lunarrender_ui_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_ui_vao);
  glGenBuffers(1, &lunarrender_ui_vbo);
  glBindVertexArray(lunarrender_ui_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_ui_vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * (GLsizei)sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * (GLsizei)sizeof(float), (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  return true;
}

static int32_t lunarrender_prepare_sky_program(void) {
  if (lunarrender_sky_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_sky_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_sky_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_sky_program = glCreateProgram();
  glAttachShader(lunarrender_sky_program, vertex);
  glAttachShader(lunarrender_sky_program, fragment);
  glLinkProgram(lunarrender_sky_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_sky_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_sky_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender sky shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_sky_program);
    lunarrender_sky_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_sky_vao);
  glBindVertexArray(lunarrender_sky_vao);
  return glGetError() == GL_NO_ERROR;
}

static void lunarrender_draw_sky(void) {
  if (!lunarrender_prepare_sky_program()) {
    return;
  }
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glUseProgram(lunarrender_sky_program);
  glBindVertexArray(lunarrender_sky_vao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}

static int32_t lunarrender_prepare_player_program(void) {
  if (lunarrender_player_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_player_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_player_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_player_program = glCreateProgram();
  glAttachShader(lunarrender_player_program, vertex);
  glAttachShader(lunarrender_player_program, fragment);
  glLinkProgram(lunarrender_player_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_player_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_player_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender player shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_player_program);
    lunarrender_player_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_player_vao);
  glGenBuffers(1, &lunarrender_player_vbo);
  glBindVertexArray(lunarrender_player_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_player_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * (GLsizei)sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * (GLsizei)sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  return true;
}

static int32_t lunarrender_prepare_ui_texture_program(void) {
  if (lunarrender_ui_texture_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_ui_texture_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_ui_texture_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_ui_texture_program = glCreateProgram();
  glAttachShader(lunarrender_ui_texture_program, vertex);
  glAttachShader(lunarrender_ui_texture_program, fragment);
  glLinkProgram(lunarrender_ui_texture_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_ui_texture_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(
        lunarrender_ui_texture_program,
        (GLsizei)sizeof(log),
        &length,
        log);
    fprintf(stderr, "LunarRender hotbar texture shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_ui_texture_program);
    lunarrender_ui_texture_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_ui_texture_vao);
  glGenBuffers(1, &lunarrender_ui_texture_vbo);
  glBindVertexArray(lunarrender_ui_texture_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_ui_texture_vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * (GLsizei)sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
      1,
      2,
      GL_FLOAT,
      GL_FALSE,
      4 * (GLsizei)sizeof(float),
      (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
  return true;
}

static int32_t lunarrender_prepare_outline_program(void) {
  if (lunarrender_outline_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_outline_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_outline_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_outline_program = glCreateProgram();
  glAttachShader(lunarrender_outline_program, vertex);
  glAttachShader(lunarrender_outline_program, fragment);
  glLinkProgram(lunarrender_outline_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_outline_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_outline_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender block outline shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_outline_program);
    lunarrender_outline_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_outline_vao);
  glGenBuffers(1, &lunarrender_outline_vbo);
  glBindVertexArray(lunarrender_outline_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_outline_vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * (GLsizei)sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  return true;
}

#ifdef LUNARRENDER_ENABLE_MMD
static int32_t lunarrender_prepare_mmd_program(void) {
  if (lunarrender_mmd_program != 0) {
    return true;
  }
  GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_mmd_vertex_shader);
  GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_mmd_fragment_shader);
  if (vertex == 0 || fragment == 0) {
    if (vertex != 0) glDeleteShader(vertex);
    if (fragment != 0) glDeleteShader(fragment);
    return false;
  }
  lunarrender_mmd_program = glCreateProgram();
  glAttachShader(lunarrender_mmd_program, vertex);
  glAttachShader(lunarrender_mmd_program, fragment);
  glLinkProgram(lunarrender_mmd_program);
  GLint linked = 0;
  glGetProgramiv(lunarrender_mmd_program, GL_LINK_STATUS, &linked);
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!linked) {
    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(lunarrender_mmd_program, (GLsizei)sizeof(log), &length, log);
    fprintf(stderr, "LunarRender MMD shader link failed: %.*s\n", (int)length, log);
    glDeleteProgram(lunarrender_mmd_program);
    lunarrender_mmd_program = 0;
    return false;
  }
  glGenVertexArrays(1, &lunarrender_mmd_vao);
  glGenBuffers(1, &lunarrender_mmd_vbo);
  glBindVertexArray(lunarrender_mmd_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_mmd_vbo);
  const GLsizei stride = 21 * (GLsizei)sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)(0 * sizeof(float)));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)(8 * sizeof(float)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void *)(12 * sizeof(float)));
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void *)(13 * sizeof(float)));
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void *)(17 * sizeof(float)));
  glEnableVertexAttribArray(6);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_mmd_vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)lunarrender_mmd_animated_vertex_count * 21 * (int)sizeof(float),
      lunarrender_mmd_animated_vertices,
      GL_STATIC_DRAW);
  return glGetError() == GL_NO_ERROR;
}
#endif

int32_t lunarrender_window_create(int width, int height) {
  if (lunarrender_window != NULL) {
    return false;
  }
  if (!glfwInit()) {
    fprintf(stderr, "LunarRender GLFW initialization failed\n");
    return false;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  lunarrender_window = glfwCreateWindow(width, height, "LunarRender", NULL, NULL);
  if (lunarrender_window == NULL) {
    fprintf(stderr, "LunarRender GLFW window creation failed\n");
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(lunarrender_window);
  glfwSwapInterval(1);
  glfwSetInputMode(lunarrender_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetCursorPos(lunarrender_window, width / 2.0, height / 2.0);
  glfwGetCursorPos(lunarrender_window, &lunarrender_cursor_x, &lunarrender_cursor_y);
  lunarrender_cursor_initialized = false;
  lunarrender_cursor_warmup_frames = 4;
  return true;
}

int32_t lunarrender_window_should_close(void) {
  return lunarrender_window == NULL || glfwWindowShouldClose(lunarrender_window) != 0;
}

void lunarrender_window_poll_events(void) {
  if (lunarrender_window == NULL) {
    return;
  }
  glfwPollEvents();
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(lunarrender_window, &x, &y);
  if (!lunarrender_cursor_initialized || lunarrender_cursor_warmup_frames > 0) {
    lunarrender_cursor_x = x;
    lunarrender_cursor_y = y;
    lunarrender_cursor_initialized = true;
    if (lunarrender_cursor_warmup_frames > 0) {
      lunarrender_cursor_warmup_frames -= 1;
    }
    lunarrender_delta_x = 0.0;
    lunarrender_delta_y = 0.0;
    return;
  }
  lunarrender_delta_x = x - lunarrender_cursor_x;
  lunarrender_delta_y = y - lunarrender_cursor_y;
  lunarrender_cursor_x = x;
  lunarrender_cursor_y = y;
}

void lunarrender_window_swap_buffers(void) {
  if (lunarrender_window != NULL) {
    glfwSwapBuffers(lunarrender_window);
  }
}

void lunarrender_window_destroy(void) {
  if (lunarrender_window != NULL) {
    glfwDestroyWindow(lunarrender_window);
    lunarrender_window = NULL;
    glfwTerminate();
  }
  lunarrender_delta_x = 0.0;
  lunarrender_delta_y = 0.0;
  lunarrender_cursor_initialized = false;
  lunarrender_cursor_warmup_frames = 0;
}

double lunarrender_window_time_seconds(void) {
  if (lunarrender_window == NULL) {
    return 0.0;
  }
  return glfwGetTime();
}

int32_t lunarrender_window_key_down(int key) {
  return lunarrender_window != NULL && glfwGetKey(lunarrender_window, key) == GLFW_PRESS;
}

int32_t lunarrender_window_mouse_button_down(int button) {
  return lunarrender_window != NULL && glfwGetMouseButton(lunarrender_window, button) == GLFW_PRESS;
}

double lunarrender_window_cursor_delta_x(void) {
  double value = lunarrender_delta_x;
  lunarrender_delta_x = 0.0;
  return value;
}

double lunarrender_window_cursor_delta_y(void) {
  double value = lunarrender_delta_y;
  lunarrender_delta_y = 0.0;
  return value;
}

#ifdef LUNARRENDER_ENABLE_MMD
static int32_t lunarrender_create_mmd_texture(void) {
  glGenTextures(1, &lunarrender_mmd_texture);
  if (lunarrender_mmd_texture == 0) {
    fprintf(stderr, "LunarRender MMD texture creation failed\n");
    return false;
  }
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, lunarrender_mmd_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      lunarrender_mmd_texture_atlas_width,
      lunarrender_mmd_texture_atlas_height,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      lunarrender_mmd_texture_atlas_rgba);
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender MMD texture upload failed\n");
    glDeleteTextures(1, &lunarrender_mmd_texture);
    lunarrender_mmd_texture = 0;
    return false;
  }
  fprintf(
      stderr,
      "LunarRender MMD texture atlas: %dx%d RGBA, linear filtering\n",
      lunarrender_mmd_texture_atlas_width,
      lunarrender_mmd_texture_atlas_height);
  return true;
}
#endif

static int32_t lunarrender_create_texture(void) {
  glGenTextures(1, &lunarrender_texture);
  if (lunarrender_texture == 0) {
    fprintf(stderr, "LunarRender texture creation failed\n");
    return false;
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, lunarrender_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      lunarrender_texture_atlas_width,
      lunarrender_texture_atlas_height,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      lunarrender_texture_atlas_rgba);
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender texture upload failed\n");
    glDeleteTextures(1, &lunarrender_texture);
    lunarrender_texture = 0;
    return false;
  }
  fprintf(
      stderr,
      "LunarRender texture atlas: %dx%d RGBA, nearest filtering\n",
      lunarrender_texture_atlas_width,
      lunarrender_texture_atlas_height);
  return true;
}

int32_t lunarrender_gl_init(void) {
  if (lunarrender_window == NULL) {
    return false;
  }
  int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
  if (version == 0) {
    fprintf(stderr, "LunarRender GLAD loading failed\n");
    return false;
  }
  const GLubyte *context_version = glGetString(GL_VERSION);
  if (context_version == NULL) {
    fprintf(stderr, "LunarRender OpenGL context version query failed\n");
    return false;
  }
  fprintf(stderr, "LunarRender OpenGL context: %s\n", (const char *)context_version);
  glEnable(GL_DEPTH_TEST);
  if (!lunarrender_create_texture()) {
    return false;
  }
#ifdef LUNARRENDER_ENABLE_MMD
  if (!lunarrender_create_mmd_texture()) {
    return false;
  }
#endif
  return true;
}

int32_t lunarrender_gl_upload_mesh(const float *vertices, int vertex_count) {
  if (lunarrender_program == 0) {
    GLuint vertex = lunarrender_compile_shader(GL_VERTEX_SHADER, lunarrender_vertex_shader);
    GLuint fragment = lunarrender_compile_shader(GL_FRAGMENT_SHADER, lunarrender_fragment_shader);
    if (vertex == 0 || fragment == 0) {
      if (vertex != 0) glDeleteShader(vertex);
      if (fragment != 0) glDeleteShader(fragment);
      return false;
    }
    lunarrender_program = glCreateProgram();
    glAttachShader(lunarrender_program, vertex);
    glAttachShader(lunarrender_program, fragment);
    glLinkProgram(lunarrender_program);
    GLint linked = 0;
    glGetProgramiv(lunarrender_program, GL_LINK_STATUS, &linked);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (!linked) {
      char log[2048];
      GLsizei length = 0;
      glGetProgramInfoLog(lunarrender_program, (GLsizei)sizeof(log), &length, log);
      fprintf(stderr, "LunarRender shader link failed: %.*s\n", (int)length, log);
      glDeleteProgram(lunarrender_program);
      lunarrender_program = 0;
      return false;
    }
    glGenVertexArrays(1, &lunarrender_vao);
    glGenBuffers(1, &lunarrender_vbo);
    glBindVertexArray(lunarrender_vao);
    glBindBuffer(GL_ARRAY_BUFFER, lunarrender_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * (GLsizei)sizeof(float), (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
  }
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertex_count * 6 * (int)sizeof(float), vertices, GL_DYNAMIC_DRAW);
  return glGetError() == GL_NO_ERROR;
}

void lunarrender_gl_draw_mesh(const float *view_projection, int vertex_count) {
  if (lunarrender_program == 0 || lunarrender_vao == 0) {
    return;
  }
  glClearColor(0.53f, 0.75f, 0.92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  lunarrender_draw_sky();
  glUseProgram(lunarrender_program);
  GLint location = glGetUniformLocation(lunarrender_program, "u_view_projection");
  glUniformMatrix4fv(location, 1, GL_FALSE, view_projection);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, lunarrender_texture);
  glUniform1i(glGetUniformLocation(lunarrender_program, "u_texture"), 0);
  glBindVertexArray(lunarrender_vao);
  glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

void lunarrender_gl_clear(void) {
  glClearColor(0.53f, 0.75f, 0.92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static LunarrenderChunkGpu *lunarrender_find_chunk(int chunk_x, int chunk_z) {
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (lunarrender_chunks[i].active && lunarrender_chunks[i].chunk_x == chunk_x &&
        lunarrender_chunks[i].chunk_z == chunk_z) {
      return &lunarrender_chunks[i];
    }
  }
  return NULL;
}

static LunarrenderChunkGpu *lunarrender_acquire_chunk(int chunk_x, int chunk_z) {
  LunarrenderChunkGpu *chunk = lunarrender_find_chunk(chunk_x, chunk_z);
  if (chunk != NULL) {
    return chunk;
  }
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (!lunarrender_chunks[i].active) {
      chunk = &lunarrender_chunks[i];
      chunk->active = true;
      chunk->chunk_x = chunk_x;
      chunk->chunk_z = chunk_z;
      chunk->vao = 0;
      chunk->vbo = 0;
      chunk->vertex_count = 0;
      chunk->opaque_vertex_count = 0;
      chunk->cutout_vertex_count = 0;
      chunk->translucent_vertex_count = 0;
      return chunk;
    }
  }
  fprintf(stderr, "LunarRender chunk GPU cache is full at %d entries\n", LUNARRENDER_MAX_CHUNKS);
  return NULL;
}

int32_t lunarrender_gl_upload_chunk_mesh(
    int chunk_x,
    int chunk_z,
    const float *vertices,
    int vertex_count,
    int opaque_vertex_count,
    int cutout_vertex_count,
    int translucent_vertex_count) {
  int64_t layer_vertex_count =
      (int64_t)opaque_vertex_count + cutout_vertex_count + translucent_vertex_count;
  if (vertex_count < 0 || opaque_vertex_count < 0 || cutout_vertex_count < 0 ||
      translucent_vertex_count < 0 || layer_vertex_count != vertex_count ||
      !lunarrender_prepare_voxel_program()) {
    return false;
  }
  LunarrenderChunkGpu *chunk = lunarrender_acquire_chunk(chunk_x, chunk_z);
  if (chunk == NULL) {
    return false;
  }
  lunarrender_configure_chunk_vao(chunk);
  glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)vertex_count * 6 * (int)sizeof(float),
      vertices,
      GL_DYNAMIC_DRAW);
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender chunk mesh upload failed at %d,%d\n", chunk_x, chunk_z);
    return false;
  }
  chunk->vertex_count = vertex_count;
  chunk->opaque_vertex_count = opaque_vertex_count;
  chunk->cutout_vertex_count = cutout_vertex_count;
  chunk->translucent_vertex_count = translucent_vertex_count;
  if (!lunarrender_chunk_upload_reported) {
    fprintf(
        stderr,
        "LunarRender chunk mesh upload: chunk=(%d,%d), vertices=%d, opaque=%d, cutout=%d, translucent=%d\n",
        chunk_x,
        chunk_z,
        vertex_count,
        opaque_vertex_count,
        cutout_vertex_count,
        translucent_vertex_count);
    lunarrender_chunk_upload_reported = true;
  }
  return true;
}

void lunarrender_gl_remove_chunk_mesh(int chunk_x, int chunk_z) {
  LunarrenderChunkGpu *chunk = lunarrender_find_chunk(chunk_x, chunk_z);
  if (chunk == NULL) {
    return;
  }
  if (chunk->vbo != 0) {
    glDeleteBuffers(1, &chunk->vbo);
  }
  if (chunk->vao != 0) {
    glDeleteVertexArrays(1, &chunk->vao);
  }
  chunk->active = false;
  chunk->vao = 0;
  chunk->vbo = 0;
  chunk->vertex_count = 0;
  chunk->opaque_vertex_count = 0;
  chunk->cutout_vertex_count = 0;
  chunk->translucent_vertex_count = 0;
}

void lunarrender_gl_draw_chunk_meshes(const float *view_projection) {
  glClearColor(0.53f, 0.75f, 0.92f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  lunarrender_draw_sky();
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  if (lunarrender_program == 0) {
    return;
  }
  glUseProgram(lunarrender_program);
  glUniformMatrix4fv(
      glGetUniformLocation(lunarrender_program, "u_view_projection"),
      1,
      GL_FALSE,
      view_projection);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, lunarrender_texture);
  glUniform1i(glGetUniformLocation(lunarrender_program, "u_texture"), 0);
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (lunarrender_chunks[i].active && lunarrender_chunks[i].opaque_vertex_count > 0) {
      glBindVertexArray(lunarrender_chunks[i].vao);
      glDrawArrays(GL_TRIANGLES, 0, lunarrender_chunks[i].opaque_vertex_count);
    }
  }
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (lunarrender_chunks[i].active && lunarrender_chunks[i].cutout_vertex_count > 0) {
      glBindVertexArray(lunarrender_chunks[i].vao);
      glDrawArrays(
          GL_TRIANGLES,
          lunarrender_chunks[i].opaque_vertex_count,
          lunarrender_chunks[i].cutout_vertex_count);
    }
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (lunarrender_chunks[i].active && lunarrender_chunks[i].translucent_vertex_count > 0) {
      glBindVertexArray(lunarrender_chunks[i].vao);
      glDrawArrays(
          GL_TRIANGLES,
          lunarrender_chunks[i].opaque_vertex_count + lunarrender_chunks[i].cutout_vertex_count,
          lunarrender_chunks[i].translucent_vertex_count);
    }
  }
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}

int32_t lunarrender_gl_upload_player_mesh(const float *vertices, int vertex_count) {
  if (vertices == NULL || vertex_count < 0 || !lunarrender_prepare_player_program()) {
    return false;
  }
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_player_vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)vertex_count * 5 * (int)sizeof(float),
      vertices,
      GL_DYNAMIC_DRAW);
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender player mesh upload failed\n");
    return false;
  }
  lunarrender_player_vertex_count = vertex_count;
  if (!lunarrender_player_upload_reported) {
    fprintf(stderr, "LunarRender player mesh upload: vertices=%d\n", vertex_count);
    lunarrender_player_upload_reported = true;
  }
  return true;
}

int32_t lunarrender_gl_replace_player_texture(
    int width,
    int height,
    const uint8_t *rgba) {
  if (width <= 0 || height <= 0 || rgba == NULL) {
    fprintf(stderr, "LunarRender player texture rejected invalid image state\n");
    return false;
  }
  if (lunarrender_player_texture == 0) {
    glGenTextures(1, &lunarrender_player_texture);
  }
  if (lunarrender_player_texture == 0) {
    fprintf(stderr, "LunarRender player texture creation failed\n");
    return false;
  }
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, lunarrender_player_texture);
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
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender player texture upload failed\n");
    return false;
  }
  fprintf(stderr, "LunarRender player skin: %dx%d RGBA, nearest filtering\n", width, height);
  return true;
}

void lunarrender_gl_draw_player_mesh(const float *view_projection) {
  if (lunarrender_player_program == 0 || lunarrender_player_vao == 0 || lunarrender_player_vertex_count <= 0) {
    return;
  }
  glUseProgram(lunarrender_player_program);
  glUniformMatrix4fv(
      glGetUniformLocation(lunarrender_player_program, "u_view_projection"),
      1,
      GL_FALSE,
      view_projection);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, lunarrender_player_texture);
  glUniform1i(glGetUniformLocation(lunarrender_player_program, "u_texture"), 2);
  glBindVertexArray(lunarrender_player_vao);
  glDrawArrays(GL_TRIANGLES, 0, lunarrender_player_vertex_count);
}

#ifdef LUNARRENDER_ENABLE_MMD
void lunarrender_gl_draw_mmd_animated(
    const float *view_projection,
    double time_seconds,
    double world_x,
    double world_y,
    double world_z) {
  if (view_projection == NULL || !lunarrender_prepare_mmd_program() || lunarrender_mmd_texture == 0) {
    return;
  }
  int frame = (int)(time_seconds * (double)lunarrender_mmd_animated_frame_rate);
  if (lunarrender_mmd_animated_frame_count > 0) {
    frame %= lunarrender_mmd_animated_frame_count;
  } else {
    frame = 0;
  }
  glUseProgram(lunarrender_mmd_program);
  glUniformMatrix4fv(
      glGetUniformLocation(lunarrender_mmd_program, "u_view_projection"),
      1,
      GL_FALSE,
      view_projection);
  glUniformMatrix4fv(
      glGetUniformLocation(lunarrender_mmd_program, "u_model_matrix"),
      1,
      GL_FALSE,
      lunarrender_mmd_animated_model_matrix);
  glUniform3f(
      glGetUniformLocation(lunarrender_mmd_program, "u_entity_offset"),
      (float)(world_x - 16.0),
      (float)(world_y - 0.02),
      (float)(world_z - 16.0));
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, lunarrender_mmd_texture);
  glUniform1i(glGetUniformLocation(lunarrender_mmd_program, "u_texture"), 1);
  const float *frame_palette = lunarrender_mmd_animated_bone_matrices +
      frame * lunarrender_mmd_animated_palette_stride;
  glBindVertexArray(lunarrender_mmd_vao);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (int i = 0; i < lunarrender_mmd_animated_mesh_count; i++) {
    const lunarrender_mmd_animated_mesh_t *mesh = &lunarrender_mmd_animated_meshes[i];
    const float *palette = frame_palette + mesh->palette_offset;
    glUniformMatrix4fv(
        glGetUniformLocation(lunarrender_mmd_program, "u_bones[0]"),
        mesh->bone_count,
        GL_FALSE,
        palette);
    glDrawArrays(GL_TRIANGLES, mesh->first_vertex, mesh->vertex_count);
  }
  if (!lunarrender_mmd_draw_reported) {
    fprintf(
        stderr,
        "LunarRender MMD draw active: %d vertices, %d batches, %d frames\n",
        lunarrender_mmd_animated_vertex_count,
        lunarrender_mmd_animated_mesh_count,
        lunarrender_mmd_animated_frame_count);
    lunarrender_mmd_draw_reported = true;
  }
  GLenum error = glGetError();
  if (error != GL_NO_ERROR && !lunarrender_mmd_gl_error_reported) {
    fprintf(stderr, "LunarRender MMD draw OpenGL error: 0x%04x\n", (unsigned int)error);
    lunarrender_mmd_gl_error_reported = true;
  }
  glDisable(GL_BLEND);
}
#endif

static void lunarrender_push_ui_vertex(
    float *vertices,
    int *offset,
    float x,
    float y,
    float red,
    float green,
    float blue) {
  vertices[(*offset)++] = x;
  vertices[(*offset)++] = y;
  vertices[(*offset)++] = red;
  vertices[(*offset)++] = green;
  vertices[(*offset)++] = blue;
}

static void lunarrender_push_ui_quad(
    float *vertices,
    int *offset,
    float left,
    float top,
    float right,
    float bottom,
    float red,
    float green,
    float blue) {
  lunarrender_push_ui_vertex(vertices, offset, left, top, red, green, blue);
  lunarrender_push_ui_vertex(vertices, offset, right, top, red, green, blue);
  lunarrender_push_ui_vertex(vertices, offset, right, bottom, red, green, blue);
  lunarrender_push_ui_vertex(vertices, offset, left, top, red, green, blue);
  lunarrender_push_ui_vertex(vertices, offset, right, bottom, red, green, blue);
  lunarrender_push_ui_vertex(vertices, offset, left, bottom, red, green, blue);
}

static void lunarrender_push_ui_texture_vertex(
    float *vertices,
    int *offset,
    float x,
    float y,
    float u,
    float v) {
  vertices[(*offset)++] = x;
  vertices[(*offset)++] = y;
  vertices[(*offset)++] = u;
  vertices[(*offset)++] = v;
}

static void lunarrender_push_ui_texture_quad(
    float *vertices,
    int *offset,
    float left,
    float top,
    float right,
    float bottom,
    int tile) {
  const float tile_width = 0.25f;
  const float padding = 0.0125f;
  const float u0 = (float)tile * tile_width + padding;
  const float u1 = (float)(tile + 1) * tile_width - padding;
  const float v0 = padding;
  const float v1 = 1.0f - padding;
  lunarrender_push_ui_texture_vertex(vertices, offset, left, top, u0, v1);
  lunarrender_push_ui_texture_vertex(vertices, offset, right, top, u1, v1);
  lunarrender_push_ui_texture_vertex(vertices, offset, right, bottom, u1, v0);
  lunarrender_push_ui_texture_vertex(vertices, offset, left, top, u0, v1);
  lunarrender_push_ui_texture_vertex(vertices, offset, right, bottom, u1, v0);
  lunarrender_push_ui_texture_vertex(vertices, offset, left, bottom, u0, v0);
}

static void lunarrender_copy_ascii_string(
    moonbit_string_t source,
    char *target,
    int target_size) {
  if (target_size <= 0) {
    return;
  }
  int length = source == NULL ? 0 : Moonbit_array_length(source);
  if (length > target_size - 1) {
    length = target_size - 1;
  }
  for (int i = 0; i < length; i++) {
    uint16_t code_unit = ((const uint16_t *)source)[i];
    target[i] = code_unit >= 32 && code_unit <= 126 ? (char)code_unit : '?';
  }
  target[length] = '\0';
}

static void lunarrender_glyph(char value, unsigned char rows[7]) {
  for (int i = 0; i < 7; i++) {
    rows[i] = 0;
  }
  switch (value) {
    case 'a': rows[0] = 0x0e; rows[1] = 0x11; rows[2] = 0x01; rows[3] = 0x0f; rows[4] = 0x11; rows[5] = 0x1f; break;
    case 'b': rows[0] = 0x10; rows[1] = 0x10; rows[2] = 0x1e; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x1e; break;
    case 'c': rows[0] = 0x0f; rows[1] = 0x10; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x0f; break;
    case 'd': rows[0] = 0x01; rows[1] = 0x01; rows[2] = 0x0f; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x0f; break;
    case 'e': rows[0] = 0x0e; rows[1] = 0x11; rows[2] = 0x1f; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x0f; break;
    case 'f': rows[0] = 0x07; rows[1] = 0x08; rows[2] = 0x1e; rows[3] = 0x08; rows[4] = 0x08; rows[5] = 0x08; break;
    case 'g': rows[0] = 0x0f; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x0f; rows[4] = 0x01; rows[5] = 0x0e; break;
    case 'h': rows[0] = 0x10; rows[1] = 0x10; rows[2] = 0x1e; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; break;
    case 'i': rows[0] = 0x04; rows[1] = 0x00; rows[2] = 0x0c; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x0e; break;
    case 'j': rows[0] = 0x02; rows[1] = 0x00; rows[2] = 0x06; rows[3] = 0x02; rows[4] = 0x12; rows[5] = 0x0c; break;
    case 'k': rows[0] = 0x10; rows[1] = 0x10; rows[2] = 0x12; rows[3] = 0x14; rows[4] = 0x18; rows[5] = 0x14; break;
    case 'l': rows[0] = 0x0c; rows[1] = 0x04; rows[2] = 0x04; rows[3] = 0x04; rows[4] = 0x04; rows[5] = 0x0e; break;
    case 'm': rows[0] = 0x1b; rows[1] = 0x15; rows[2] = 0x15; rows[3] = 0x15; rows[4] = 0x15; rows[5] = 0x15; break;
    case 'n': rows[0] = 0x1e; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x11; break;
    case 'o': rows[0] = 0x0e; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x11; rows[5] = 0x0e; break;
    case 'p': rows[0] = 0x1e; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x1e; rows[4] = 0x10; rows[5] = 0x10; break;
    case 'q': rows[0] = 0x0f; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x0f; rows[4] = 0x01; rows[5] = 0x01; break;
    case 'r': rows[0] = 0x1e; rows[1] = 0x11; rows[2] = 0x10; rows[3] = 0x10; rows[4] = 0x10; rows[5] = 0x10; break;
    case 's': rows[0] = 0x0f; rows[1] = 0x10; rows[2] = 0x0e; rows[3] = 0x01; rows[4] = 0x01; rows[5] = 0x1e; break;
    case 't': rows[0] = 0x08; rows[1] = 0x08; rows[2] = 0x1e; rows[3] = 0x08; rows[4] = 0x08; rows[5] = 0x07; break;
    case 'u': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x13; rows[5] = 0x0d; break;
    case 'v': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x11; rows[4] = 0x0a; rows[5] = 0x04; break;
    case 'w': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x11; rows[3] = 0x15; rows[4] = 0x15; rows[5] = 0x0a; break;
    case 'x': rows[0] = 0x11; rows[1] = 0x0a; rows[2] = 0x04; rows[3] = 0x0a; rows[4] = 0x11; break;
    case 'y': rows[0] = 0x11; rows[1] = 0x11; rows[2] = 0x0a; rows[3] = 0x04; rows[4] = 0x08; rows[5] = 0x10; break;
    case 'z': rows[0] = 0x1f; rows[1] = 0x02; rows[2] = 0x04; rows[3] = 0x08; rows[4] = 0x10; rows[5] = 0x1f; break;
    case '/': rows[0] = 0x01; rows[1] = 0x02; rows[2] = 0x04; rows[3] = 0x08; rows[4] = 0x10; break;
    case '-': rows[2] = 0x1f; break;
    case ':': rows[1] = 0x04; rows[4] = 0x04; break;
    case '?': rows[0] = 0x0e; rows[1] = 0x11; rows[2] = 0x02; rows[3] = 0x04; rows[5] = 0x04; break;
    default:
      if (value >= '0' && value <= '9') {
        static const unsigned char digits[10][6] = {
            {0x0e, 0x11, 0x13, 0x15, 0x19, 0x0e},
            {0x04, 0x0c, 0x04, 0x04, 0x04, 0x0e},
            {0x0e, 0x11, 0x01, 0x06, 0x08, 0x1f},
            {0x1e, 0x01, 0x01, 0x06, 0x01, 0x1e},
            {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02},
            {0x1f, 0x10, 0x1e, 0x01, 0x11, 0x0e},
            {0x06, 0x08, 0x10, 0x1e, 0x11, 0x0e},
            {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08},
            {0x0e, 0x11, 0x0e, 0x11, 0x11, 0x0e},
            {0x0e, 0x11, 0x0f, 0x01, 0x02, 0x1c},
        };
        for (int i = 0; i < 6; i++) rows[i] = digits[value - '0'][i];
      }
      break;
  }
}

static void lunarrender_push_ui_text(
    float *vertices,
    int *offset,
    const char *text,
    float x,
    float y,
    float scale,
    float red,
    float green,
    float blue) {
  if (text == NULL) return;
  for (int index = 0; text[index] != '\0' && index < 96; index++) {
    unsigned char rows[7];
    lunarrender_glyph(text[index], rows);
    for (int row = 0; row < 7; row++) {
      for (int column = 0; column < 5; column++) {
        if ((rows[row] & (1 << (4 - column))) != 0) {
          float cell_x = x + (float)index * scale * 6.0f + (float)column * scale;
          float cell_y = y + (float)row * scale;
          lunarrender_push_ui_quad(
              vertices,
              offset,
              cell_x,
              cell_y,
              cell_x + scale,
              cell_y + scale,
              red,
              green,
              blue);
        }
      }
    }
  }
}

void lunarrender_gl_draw_command_overlay(
    int open,
    moonbit_string_t line,
    moonbit_string_t status,
    int width,
    int height) {
  if (width <= 0 || height <= 0 || !lunarrender_prepare_ui_program()) return;
  char line_text[97];
  char status_text[97];
  lunarrender_copy_ascii_string(line, line_text, (int)sizeof(line_text));
  lunarrender_copy_ascii_string(status, status_text, (int)sizeof(status_text));
  static float vertices[220000];
  int offset = 0;
  if (status_text[0] != '\0') {
    lunarrender_push_ui_quad(vertices, &offset, 18.0f, 18.0f, (float)width - 18.0f, 48.0f, 0.05f, 0.08f, 0.10f);
    lunarrender_push_ui_text(vertices, &offset, status_text, 28.0f, 25.0f, 2.0f, 0.95f, 0.92f, 0.72f);
  }
  if (open) {
    lunarrender_push_ui_quad(vertices, &offset, 18.0f, (float)height - 94.0f, (float)width - 18.0f, (float)height - 54.0f, 0.03f, 0.04f, 0.05f);
    lunarrender_push_ui_quad(vertices, &offset, 22.0f, (float)height - 90.0f, (float)width - 22.0f, (float)height - 58.0f, 0.13f, 0.16f, 0.19f);
    lunarrender_push_ui_text(vertices, &offset, line_text, 30.0f, (float)height - 84.0f, 3.0f, 1.0f, 1.0f, 1.0f);
  }
  if (offset == 0) return;
  glUseProgram(lunarrender_ui_program);
  glUniform2f(glGetUniformLocation(lunarrender_ui_program, "u_viewport"), (float)width, (float)height);
  glBindVertexArray(lunarrender_ui_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_ui_vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)offset * (GLint)sizeof(float), vertices, GL_DYNAMIC_DRAW);
  glDisable(GL_DEPTH_TEST);
  glDrawArrays(GL_TRIANGLES, 0, offset / 5);
  glEnable(GL_DEPTH_TEST);
}

static void lunarrender_draw_pixel_icon(
    float *vertices,
    int *offset,
    float x,
    float y,
    float cell,
    const unsigned char *rows,
    float red,
    float green,
    float blue) {
  for (int row = 0; row < 7; row++) {
    for (int column = 0; column < 5; column++) {
      if ((rows[row] & (1 << (4 - column))) != 0) {
        lunarrender_push_ui_quad(
            vertices,
            offset,
            x + (float)column * cell,
            y + (float)row * cell,
            x + (float)(column + 1) * cell,
            y + (float)(row + 1) * cell,
            red,
            green,
            blue);
      }
    }
  }
}

static void lunarrender_draw_crosshair(float *vertices, int *offset, int width, int height) {
  float center_x = (float)width * 0.5f;
  float center_y = (float)height * 0.5f;
  lunarrender_push_ui_quad(vertices, offset, center_x - 1.0f, center_y - 8.0f, center_x + 1.0f, center_y + 8.0f, 0.02f, 0.02f, 0.02f);
  lunarrender_push_ui_quad(vertices, offset, center_x - 8.0f, center_y - 1.0f, center_x + 8.0f, center_y + 1.0f, 0.02f, 0.02f, 0.02f);
  lunarrender_push_ui_quad(vertices, offset, center_x - 0.5f, center_y - 7.0f, center_x + 0.5f, center_y + 7.0f, 0.95f, 0.95f, 0.95f);
  lunarrender_push_ui_quad(vertices, offset, center_x - 7.0f, center_y - 0.5f, center_x + 7.0f, center_y + 0.5f, 0.95f, 0.95f, 0.95f);
}

static void lunarrender_draw_status_icons(float *vertices, int *offset, int width, int height) {
  static const unsigned char heart[7] = {0x00, 0x0a, 0x1f, 0x1f, 0x0e, 0x04, 0x00};
  static const unsigned char shield[7] = {0x0e, 0x1f, 0x1f, 0x0e, 0x0e, 0x04, 0x00};
  static const unsigned char hunger[7] = {0x06, 0x0f, 0x1f, 0x1e, 0x0c, 0x08, 0x00};
  float icon_y = (float)height - 116.0f;
  for (int i = 0; i < 10; i++) {
    float x = 20.0f + (float)i * 15.0f;
    lunarrender_draw_pixel_icon(vertices, offset, x, icon_y, 2.0f, shield, 0.48f, 0.52f, 0.57f);
  }
  icon_y = (float)height - 96.0f;
  for (int i = 0; i < 10; i++) {
    float x = 20.0f + (float)i * 15.0f;
    lunarrender_draw_pixel_icon(vertices, offset, x, icon_y, 2.0f, heart, 0.88f, 0.12f, 0.12f);
    float hunger_x = (float)width - 170.0f + (float)i * 15.0f;
    lunarrender_draw_pixel_icon(vertices, offset, hunger_x, icon_y, 2.0f, hunger, 0.92f, 0.55f, 0.12f);
  }
}

void lunarrender_gl_draw_hotbar(int selected_slot, int width, int height) {
  if (width <= 0 || height <= 0 || !lunarrender_prepare_ui_program()) {
    return;
  }
  if (selected_slot < 0 || selected_slot > 8) {
    selected_slot = 0;
  }
  float vertices[24000];
  int offset = 0;
  const float slot_size = 40.0f;
  const float gap = 2.0f;
  const float total_width = 9.0f * slot_size + 8.0f * gap;
  const float left = ((float)width - total_width) * 0.5f;
  const float top = (float)height - 58.0f;
  const float colors[9][3] = {
      {0.55f, 0.58f, 0.61f}, {0.45f, 0.26f, 0.13f}, {0.25f, 0.58f, 0.18f},
      {0.55f, 0.58f, 0.61f}, {0.45f, 0.26f, 0.13f}, {0.25f, 0.58f, 0.18f},
      {0.55f, 0.58f, 0.61f}, {0.45f, 0.26f, 0.13f}, {0.25f, 0.58f, 0.18f},
  };
  lunarrender_push_ui_quad(vertices, &offset, left - 7.0f, top - 7.0f, left + total_width + 7.0f, top + slot_size + 7.0f, 0.03f, 0.04f, 0.05f);
  for (int i = 0; i < 9; i++) {
    float x = left + (slot_size + gap) * (float)i;
    float border_red = i == selected_slot ? 1.0f : 0.22f;
    float border_green = i == selected_slot ? 0.84f : 0.24f;
    float border_blue = i == selected_slot ? 0.24f : 0.28f;
    lunarrender_push_ui_quad(vertices, &offset, x, top, x + slot_size, top + slot_size, border_red, border_green, border_blue);
    lunarrender_push_ui_quad(vertices, &offset, x + 3.0f, top + 3.0f, x + slot_size - 3.0f, top + slot_size - 3.0f, 0.10f, 0.11f, 0.12f);
    lunarrender_push_ui_quad(vertices, &offset, x + 10.0f, top + 10.0f, x + 30.0f, top + 30.0f, colors[i][0], colors[i][1], colors[i][2]);
    lunarrender_push_ui_quad(vertices, &offset, x + 10.0f, top + 10.0f, x + 30.0f, top + 13.0f, colors[i][0] + 0.15f, colors[i][1] + 0.15f, colors[i][2] + 0.15f);
    char number[2] = {(char)('1' + i), '\0'};
    lunarrender_push_ui_text(vertices, &offset, number, x + 3.0f, top + 3.0f, 1.4f, 0.95f, 0.95f, 0.95f);
  }
  lunarrender_draw_status_icons(vertices, &offset, width, height);
  lunarrender_draw_crosshair(vertices, &offset, width, height);
  float experience_left = (float)width * 0.5f - 180.0f;
  float experience_top = (float)height - 72.0f;
  lunarrender_push_ui_quad(vertices, &offset, experience_left, experience_top, experience_left + 360.0f, experience_top + 5.0f, 0.02f, 0.04f, 0.02f);
  lunarrender_push_ui_quad(vertices, &offset, experience_left + 2.0f, experience_top + 1.0f, experience_left + 178.0f, experience_top + 4.0f, 0.24f, 0.86f, 0.20f);
  glUseProgram(lunarrender_ui_program);
  glUniform2f(glGetUniformLocation(lunarrender_ui_program, "u_viewport"), (float)width, (float)height);
  glBindVertexArray(lunarrender_ui_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_ui_vbo);
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)offset * (GLint)sizeof(float), vertices, GL_DYNAMIC_DRAW);
  glDisable(GL_DEPTH_TEST);
  glDrawArrays(GL_TRIANGLES, 0, offset / 5);
  if (lunarrender_prepare_ui_texture_program()) {
    float texture_vertices[9 * 6 * 4];
    int texture_offset = 0;
    for (int i = 0; i < 9; i++) {
      float x = left + (slot_size + gap) * (float)i;
      int tile = i % 3 == 0 ? 0 : (i % 3 == 1 ? 1 : 3);
      lunarrender_push_ui_texture_quad(
          texture_vertices,
          &texture_offset,
          x + 8.0f,
          top + 8.0f,
          x + slot_size - 8.0f,
          top + slot_size - 8.0f,
          tile);
    }
    glUseProgram(lunarrender_ui_texture_program);
    glUniform2f(
        glGetUniformLocation(lunarrender_ui_texture_program, "u_viewport"),
        (float)width,
        (float)height);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lunarrender_texture);
    glUniform1i(glGetUniformLocation(lunarrender_ui_texture_program, "u_texture"), 0);
    glBindVertexArray(lunarrender_ui_texture_vao);
    glBindBuffer(GL_ARRAY_BUFFER, lunarrender_ui_texture_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)texture_offset * (GLint)sizeof(float),
        texture_vertices,
        GL_DYNAMIC_DRAW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, texture_offset / 4);
    glDisable(GL_BLEND);
  }
  glEnable(GL_DEPTH_TEST);
}

void lunarrender_gl_draw_block_outline(
    const float *view_projection,
    int block_x,
    int block_y,
    int block_z) {
  if (view_projection == NULL || !lunarrender_prepare_outline_program()) {
    return;
  }
  const float expand = 0.002f;
  const float min_x = (float)block_x - expand;
  const float min_y = (float)block_y - expand;
  const float min_z = (float)block_z - expand;
  const float max_x = (float)block_x + 1.0f + expand;
  const float max_y = (float)block_y + 1.0f + expand;
  const float max_z = (float)block_z + 1.0f + expand;
  const float vertices[24 * 3] = {
      min_x, min_y, min_z, max_x, min_y, min_z,
      min_x, max_y, min_z, max_x, max_y, min_z,
      min_x, min_y, max_z, max_x, min_y, max_z,
      min_x, max_y, max_z, max_x, max_y, max_z,
      min_x, min_y, min_z, min_x, max_y, min_z,
      max_x, min_y, min_z, max_x, max_y, min_z,
      min_x, min_y, max_z, min_x, max_y, max_z,
      max_x, min_y, max_z, max_x, max_y, max_z,
      min_x, min_y, min_z, min_x, min_y, max_z,
      max_x, min_y, min_z, max_x, min_y, max_z,
      min_x, max_y, min_z, min_x, max_y, max_z,
      max_x, max_y, min_z, max_x, max_y, max_z,
  };
  glUseProgram(lunarrender_outline_program);
  glUniformMatrix4fv(
      glGetUniformLocation(lunarrender_outline_program, "u_view_projection"),
      1,
      GL_FALSE,
      view_projection);
  glBindVertexArray(lunarrender_outline_vao);
  glBindBuffer(GL_ARRAY_BUFFER, lunarrender_outline_vbo);
  glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)sizeof(vertices),
      vertices,
      GL_STREAM_DRAW);
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0f);
  glDrawArrays(GL_LINES, 0, 24);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}

int32_t lunarrender_gl_replace_texture_atlas(
    int width,
    int height,
    const uint8_t *rgba) {
  if (lunarrender_texture == 0 || width <= 0 || height <= 0 || rgba == NULL) {
    fprintf(stderr, "LunarRender texture replacement rejected invalid image state\n");
    return false;
  }
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, lunarrender_texture);
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
  if (glGetError() != GL_NO_ERROR) {
    fprintf(stderr, "LunarRender texture replacement failed at OpenGL upload\n");
    return false;
  }
  return true;
}

void lunarrender_gl_destroy_renderer(void) {
  for (int i = 0; i < LUNARRENDER_MAX_CHUNKS; i++) {
    if (lunarrender_chunks[i].active) {
      if (lunarrender_chunks[i].vbo != 0) {
        glDeleteBuffers(1, &lunarrender_chunks[i].vbo);
      }
      if (lunarrender_chunks[i].vao != 0) {
        glDeleteVertexArrays(1, &lunarrender_chunks[i].vao);
      }
      lunarrender_chunks[i].active = false;
      lunarrender_chunks[i].vbo = 0;
      lunarrender_chunks[i].vao = 0;
      lunarrender_chunks[i].opaque_vertex_count = 0;
      lunarrender_chunks[i].cutout_vertex_count = 0;
      lunarrender_chunks[i].translucent_vertex_count = 0;
    }
  }
  if (lunarrender_texture != 0) {
    glDeleteTextures(1, &lunarrender_texture);
    lunarrender_texture = 0;
  }
  if (lunarrender_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_vbo);
    lunarrender_vbo = 0;
  }
  if (lunarrender_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_vao);
    lunarrender_vao = 0;
  }
  if (lunarrender_program != 0) {
    glDeleteProgram(lunarrender_program);
    lunarrender_program = 0;
  }
  if (lunarrender_ui_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_ui_vbo);
    lunarrender_ui_vbo = 0;
  }
  if (lunarrender_ui_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_ui_vao);
    lunarrender_ui_vao = 0;
  }
  if (lunarrender_ui_program != 0) {
    glDeleteProgram(lunarrender_ui_program);
    lunarrender_ui_program = 0;
  }
  if (lunarrender_ui_texture_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_ui_texture_vbo);
    lunarrender_ui_texture_vbo = 0;
  }
  if (lunarrender_ui_texture_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_ui_texture_vao);
    lunarrender_ui_texture_vao = 0;
  }
  if (lunarrender_ui_texture_program != 0) {
    glDeleteProgram(lunarrender_ui_texture_program);
    lunarrender_ui_texture_program = 0;
  }
  if (lunarrender_outline_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_outline_vbo);
    lunarrender_outline_vbo = 0;
  }
  if (lunarrender_outline_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_outline_vao);
    lunarrender_outline_vao = 0;
  }
  if (lunarrender_outline_program != 0) {
    glDeleteProgram(lunarrender_outline_program);
    lunarrender_outline_program = 0;
  }
  if (lunarrender_sky_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_sky_vbo);
    lunarrender_sky_vbo = 0;
  }
  if (lunarrender_sky_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_sky_vao);
    lunarrender_sky_vao = 0;
  }
  if (lunarrender_sky_program != 0) {
    glDeleteProgram(lunarrender_sky_program);
    lunarrender_sky_program = 0;
  }
  if (lunarrender_player_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_player_vbo);
    lunarrender_player_vbo = 0;
  }
  if (lunarrender_player_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_player_vao);
    lunarrender_player_vao = 0;
  }
  if (lunarrender_player_program != 0) {
    glDeleteProgram(lunarrender_player_program);
    lunarrender_player_program = 0;
  }
  if (lunarrender_player_texture != 0) {
    glDeleteTextures(1, &lunarrender_player_texture);
    lunarrender_player_texture = 0;
  }
  lunarrender_player_vertex_count = 0;
  lunarrender_chunk_upload_reported = false;
  lunarrender_player_upload_reported = false;
#ifdef LUNARRENDER_ENABLE_MMD
  if (lunarrender_mmd_texture != 0) {
    glDeleteTextures(1, &lunarrender_mmd_texture);
    lunarrender_mmd_texture = 0;
  }
  if (lunarrender_mmd_vbo != 0) {
    glDeleteBuffers(1, &lunarrender_mmd_vbo);
    lunarrender_mmd_vbo = 0;
  }
  if (lunarrender_mmd_vao != 0) {
    glDeleteVertexArrays(1, &lunarrender_mmd_vao);
    lunarrender_mmd_vao = 0;
  }
  if (lunarrender_mmd_program != 0) {
    glDeleteProgram(lunarrender_mmd_program);
    lunarrender_mmd_program = 0;
  }
  lunarrender_mmd_draw_reported = false;
  lunarrender_mmd_gl_error_reported = false;
#endif
}
