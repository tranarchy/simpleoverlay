#include <string.h>

#include "../include/glad.h"
#include "../include/microui.h"

#include "atlas.h"

#define BUFFER_SIZE 16384

#define _MAT4_INDEX(row, column) ((row) + (column) * 4)

static const GLchar* vertexShaderSource =
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

static const GLchar* fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 ourTexCoord;\n"
    "in vec4 ourColor;\n"
    "uniform sampler2D ourTexture;\n"
    "void main() {\n"
    "   vec4 texColor = texture(ourTexture, ourTexCoord);\n"
    "   FragColor = vec4(ourColor.rgb, ourColor.a * texColor.r);\n"
    "}\n";

static GLfloat vertices[BUFFER_SIZE * 32];
static GLuint  index_buf[BUFFER_SIZE * 6];

static GLfloat projection_matrix[16];

static GLint shaderProgram;
static GLuint ebo, vao, vbo, id;

static int prev_size[2];

static int buf_idx, projection_location, texture_location;

static void mat4_mult(const GLfloat lhs[16], const GLfloat rhs[16], GLfloat result[16])
{
	int c, r, i;

	for (c = 0; c < 4; c++)
	{
		for (r = 0; r < 4; r++)
		{
			result[_MAT4_INDEX(r, c)] = 0.0f;

			for (i = 0; i < 4; i++)
				result[_MAT4_INDEX(r, c)] += lhs[_MAT4_INDEX(r, i)] * rhs[_MAT4_INDEX(i, c)];
		}
	}
}

static void get_projection_matrix(int width, int height)
{
	const GLfloat left = 0.0f;
	const GLfloat right = (GLfloat)width;
	const GLfloat bottom = (GLfloat)height;
	const GLfloat top = 0.0f;
	const GLfloat zNear = -1.0f;
	const GLfloat zFar = 1.0f;

	const GLfloat projection[16] = {
		(2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
		0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
		0.0f, 0.0f, (-2.0f / (zFar - zNear)), 0.0f,

		-((right + left) / (right - left)),
		-((top + bottom) / (top - bottom)),
		-((zFar + zNear) / (zFar - zNear)),
		1.0f,
	};

	memcpy(projection_matrix, projection, 16 * sizeof(GLfloat));
}

void gl_init(void) {
  GLint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);
  
  GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
 
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
 
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buf), index_buf, GL_DYNAMIC_DRAW);

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

  for(int i=0; i<128; i++) {
    for(int j=0; j<128; j++) {
      if(atlas_texture[i][j]==' ') {
        atlas_texture[i][j] = 0;
      } else {
        atlas_texture[i][j] = -1;
      }
    }
  }

  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
    GL_RED, GL_UNSIGNED_BYTE, atlas_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  projection_location = glGetUniformLocation(shaderProgram, "projection");
  texture_location = glGetUniformLocation(shaderProgram, "ourTexture");
   
  glUniform1i(texture_location, 0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(shaderProgram);
}

void gl_flush(unsigned int* viewport, float scale) {
  if (buf_idx == 0) { return; }
  
  int width = viewport[2];
  int height = viewport[3];

  if (width != prev_size[0] || height != prev_size[1]) {
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
  
    get_projection_matrix(viewport[2], viewport[3]);

    const GLfloat model[16] = {
      scale, 0.0f, 0.0f, 0.0f,
      0.0f, scale, 0.0f, 0.0f,
      0.0f, 0.0f,  scale, 0.0f,
      0, 0, 0.0f, 1.0f,
    };

    GLfloat mvp[16];
    mat4_mult(projection_matrix, model, mvp);

    glUniformMatrix4fv(projection_location, 1, GL_FALSE, mvp);

  }

  prev_size[0] = width;
  prev_size[1] = height;

  glBindVertexArray(vao);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, buf_idx * 32 * sizeof(GLfloat), vertices);

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

  vertices[vertex_idx++] = dst.x;
  vertices[vertex_idx++] = dst.y;

  vertices[vertex_idx++] = r;
  vertices[vertex_idx++] = g;
  vertices[vertex_idx++] = b;
  vertices[vertex_idx++] = a;

  vertices[vertex_idx++] = x;
  vertices[vertex_idx++] = y;

  vertices[vertex_idx++] = dst.x + dst.w;
  vertices[vertex_idx++] = dst.y;

  vertices[vertex_idx++] = r;
  vertices[vertex_idx++] = g;
  vertices[vertex_idx++] = b;
  vertices[vertex_idx++] = a;

  vertices[vertex_idx++] = x + w;
  vertices[vertex_idx++] = y;

  vertices[vertex_idx++] = dst.x;
  vertices[vertex_idx++] = dst.y + dst.h;

  vertices[vertex_idx++] = r;
  vertices[vertex_idx++] = g;
  vertices[vertex_idx++] = b;
  vertices[vertex_idx++] = a;

  vertices[vertex_idx++] = x;
  vertices[vertex_idx++] = y + h;

  vertices[vertex_idx++] = dst.x + dst.w;
  vertices[vertex_idx++] = dst.y + dst.h;

  vertices[vertex_idx++] = r;
  vertices[vertex_idx++] = g;
  vertices[vertex_idx++] = b;
  vertices[vertex_idx++] = a;

  vertices[vertex_idx++] = x + w;
  vertices[vertex_idx++] = y + h;

  index_buf[index_idx++] = element_idx + 0;
  index_buf[index_idx++] = element_idx + 1;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 3;
  index_buf[index_idx++] = element_idx + 1;
}

int gl_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}

int gl_get_text_height(void) {
  return 18;
}

void gl_draw_rect(mu_Rect rect, mu_Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}

void gl_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
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
