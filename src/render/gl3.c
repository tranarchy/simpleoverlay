#include <string.h>

#include <glad.h>
#include <common.h>
#include <microui.h>

#include "atlas.h"

#define BUFFER_SIZE 16384

void get_projection_matrix(int width, int height, GLfloat *projection_matrix);
void mat4_mult(const GLfloat lhs[16], const GLfloat rhs[16], GLfloat result[16]);

extern s_config config;

static const GLchar* vertex_shader_source_330 =
    "#version 330 core\n"
    "layout (location = 0) in vec2 position;\n"
    "layout (location = 1) in vec4 color;\n"
    "layout (location = 2) in vec2 texCoord;\n"
    "out vec4 ourColor;\n"
    "out vec2 ourTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0f, 1.0f);\n"
    "    ourColor = color;\n"
    "    ourTexCoord = texCoord;\n"
    "}\n";

static const GLchar* fragment_shader_source_330 =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 ourTexCoord;\n"
    "in vec4 ourColor;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "   vec4 texColor = texture(ourTexture, ourTexCoord);\n"
    "   FragColor = ourColor * texColor;\n"
    "}\n";

static const GLchar* vertex_shader_source_130 =
    "#version 130\n"
    "in vec2 position;\n"
    "in vec4 color;\n"
    "in vec2 texCoord;\n"
    "out vec4 ourColor;\n"
    "out vec2 ourTexCoord;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0f, 1.0f);\n"
    "    ourColor = color;\n"
    "    ourTexCoord = texCoord;\n"
    "}\n";

static const GLchar* fragment_shader_source_130 =
    "#version 130\n"
    "out vec4 FragColor;\n"
    "in vec2 ourTexCoord;\n"
    "in vec4 ourColor;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "   vec4 texColor = texture(ourTexture, ourTexCoord);\n"
    "   FragColor = ourColor * texColor;\n"
    "}\n";

static GLfloat vert_buf[BUFFER_SIZE * 32];
static GLuint  index_buf[BUFFER_SIZE * 6];

static GLfloat projection_matrix[16];

static GLint shader_program;
static GLuint ebo, vao, vbo, id;

static mu_Rect prev_rect;

static int buf_idx, projection_location, texture_location;

static int prev_width = 0;
static int prev_height = 0;

void gl3_init(void) {
  int gl_minor_version, gl_major_version;

  const void *fragment_shader_source;
  const void *vertex_shader_source;

  glGetIntegerv(GL_MAJOR_VERSION, &gl_major_version);
  glGetIntegerv(GL_MINOR_VERSION, &gl_minor_version);

  if (gl_major_version > 3 || gl_minor_version == 3) {
      vertex_shader_source = &vertex_shader_source_330;
      fragment_shader_source = &fragment_shader_source_330;
  } else {
      vertex_shader_source = &vertex_shader_source_130;
      fragment_shader_source = &fragment_shader_source_130;
  }

  GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  
  GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
 
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
 
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert_buf), vert_buf, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buf), index_buf, GL_DYNAMIC_DRAW);

  glBindAttribLocation(shader_program, 0, "position");
  glBindAttribLocation(shader_program, 1, "color");
  glBindAttribLocation(shader_program, 2, "texCoord");

  // pos
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // color
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // texture
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  GLubyte rgba_texture[ATLAS_WIDTH * ATLAS_HEIGHT * 4];

  for(int i = 0; i < ATLAS_HEIGHT; i++) {
    for(int j = 0; j < ATLAS_WIDTH; j++) {
      if(atlas_texture[i][j]==' ') {
        atlas_texture[i][j] = 0;
      } else {
        atlas_texture[i][j] = -1;
      }

      int rgba_index = (i * ATLAS_WIDTH + j) * 4;

      GLubyte atlas_cur = atlas_texture[i][j];

      rgba_texture[rgba_index++] = atlas_cur;
      rgba_texture[rgba_index++] = atlas_cur;
      rgba_texture[rgba_index++] = atlas_cur;
      rgba_texture[rgba_index++] = atlas_cur;
      
    }
  }

  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, rgba_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  projection_location = glGetUniformLocation(shader_program, "projection");
  texture_location = glGetUniformLocation(shader_program, "ourTexture");
   
  glUniform1i(texture_location, 0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(shader_program);
}


void gl3_flush(mu_Rect rect, unsigned int* viewport) {
  if (buf_idx == 0) { return; }

  GLfloat x, y;
  
  int width = viewport[2];
  int height = viewport[3];

  if (width != prev_width || height != prev_height || rect.w != prev_rect.w || rect.h != prev_rect.h) {
    glViewport(viewport[0], viewport[1], width, height);
    
    get_projection_matrix(width, height, projection_matrix);

    switch (config.pos_x) {
        case LEFT_X: x = 5; break;
        case RIGHT_X: x = width - ((rect.w + 5) * config.scale); break;
        case CENTER_X: x = (width / 2) - ((rect.w * config.scale) / 2); break; 
    }

    switch (config.pos_y) {
        case TOP_Y: y = 5; break;
        case BOTTOM_Y: y = height - ((rect.h + 5) * config.scale); break;
        case CENTER_Y: y = (height / 2) - ((rect.h * config.scale) / 2); break; 
    }

    const GLfloat model[16] = {
      config.scale, 0.0f, 0.0f, 0.0f,
      0.0f, config.scale, 0.0f, 0.0f,
      0.0f, 0.0f,  config.scale, 0.0f,
      x, y, 0.0f, 1.0f,
    };

    GLfloat mvp[16];
    mat4_mult(projection_matrix, model, mvp);

    glUniformMatrix4fv(projection_location, 1, GL_FALSE, mvp);
  }

  prev_width = width;
  prev_height = height;
  prev_rect = rect;

  glBindVertexArray(vao);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vert_buf), NULL, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, buf_idx * 32 * sizeof(GLfloat), vert_buf);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buf), NULL, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buf_idx * 6 * sizeof(GLuint), index_buf);

  glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  buf_idx = 0;
}

static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
  int vertex_idx = buf_idx * 32;
  int element_idx = buf_idx * 4;
  int   index_idx = buf_idx * 6;
  buf_idx++;

  float r = color.r / 255.0f;
  float g = color.g / 255.0f;
  float b = color.b / 255.0f;
  float a = color.a / 255.0f;

  float x = src.x / (float) ATLAS_WIDTH;
  float y = src.y / (float) ATLAS_HEIGHT;
  float w = src.w / (float) ATLAS_WIDTH;
  float h = src.h / (float) ATLAS_HEIGHT;

  vert_buf[vertex_idx++] = dst.x;
  vert_buf[vertex_idx++] = dst.y;

  vert_buf[vertex_idx++] = r;
  vert_buf[vertex_idx++] = g;
  vert_buf[vertex_idx++] = b;
  vert_buf[vertex_idx++] = a;

  vert_buf[vertex_idx++] = x;
  vert_buf[vertex_idx++] = y;

  vert_buf[vertex_idx++] = dst.x + dst.w;
  vert_buf[vertex_idx++] = dst.y;

  vert_buf[vertex_idx++] = r;
  vert_buf[vertex_idx++] = g;
  vert_buf[vertex_idx++] = b;
  vert_buf[vertex_idx++] = a;

  vert_buf[vertex_idx++] = x + w;
  vert_buf[vertex_idx++] = y;

  vert_buf[vertex_idx++] = dst.x;
  vert_buf[vertex_idx++] = dst.y + dst.h;

  vert_buf[vertex_idx++] = r;
  vert_buf[vertex_idx++] = g;
  vert_buf[vertex_idx++] = b;
  vert_buf[vertex_idx++] = a;

  vert_buf[vertex_idx++] = x;
  vert_buf[vertex_idx++] = y + h;

  vert_buf[vertex_idx++] = dst.x + dst.w;
  vert_buf[vertex_idx++] = dst.y + dst.h;

  vert_buf[vertex_idx++] = r;
  vert_buf[vertex_idx++] = g;
  vert_buf[vertex_idx++] = b;
  vert_buf[vertex_idx++] = a;

  vert_buf[vertex_idx++] = x + w;
  vert_buf[vertex_idx++] = y + h;

  index_buf[index_idx++] = element_idx + 0;
  index_buf[index_idx++] = element_idx + 1;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 3;
  index_buf[index_idx++] = element_idx + 1;
}

int gl3_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}

int gl3_get_text_height(void) {
  return 18;
}

void gl3_draw_rect(mu_Rect rect, mu_Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}

void gl3_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
  mu_Rect dst = { pos.x, pos.y, 0, 0 };
  for (const char *p = text; *p; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    mu_Rect src = atlas[ATLAS_FONT + chr];
    dst.w = src.w;
    dst.h = src.h;
    push_quad(dst, src, color);
    dst.x += dst.w;
  }
}
