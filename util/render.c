#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT

#include <time.h>
#include <stdio.h>
#include <stdarg.h>

#include "../include/glad.h"
#include "../include/gltext.h"
#include "../include/common.h"

void populate_mem(s_overlay_info *overlay_info);
void populate_cpu(s_overlay_info *s_overlay_info);
void populate_amdgpu(s_overlay_info *overlay_info);

extern s_config config;

typedef struct s_texts_info {
    GLTtext *key_texts[8];
    GLTtext *value_texts[8];

    GLfloat pos_y;
    GLfloat max_key_width;

    int text_count;
} s_texts_info;

static unsigned int prev_viewport[2] = { 0 }; 

static s_overlay_info overlay_info = { 0 };

static s_texts_info texts_info = { 
  .key_texts = { NULL },
  .value_texts = { NULL },
  .pos_y = 10.0f,
  .max_key_width = 0.0f,
  .text_count = 0 
};

static int frames;

static long long prev_time;
static long long prev_time_frametime;

static long long get_time_nano(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1e9 + ts.tv_nsec;
}

static void populate_overlay_info(void) {
    long long cur_time = get_time_nano();

    frames++;

    if (cur_time - prev_time >= 1e9) {
      overlay_info.fps = frames;
      overlay_info.frametime = (cur_time - prev_time_frametime) / 1e6f;

      if (!config.fps_only) {
        populate_cpu(&overlay_info);
        populate_mem(&overlay_info);

        if (strcmp((const char*)glGetString(GL_VENDOR), "AMD") == 0) {
           populate_amdgpu(&overlay_info);
        }
      }

      frames = 0;
      prev_time = cur_time;
    }

    prev_time_frametime = cur_time;
}

static void add_text(const char *key, const char *value, ...) { 
    va_list args;
    char value_buffer[256];
    GLfloat key_width;

    bool found_max_key_width = false; 
    
    int text_count = texts_info.text_count;

    GLTtext **key_text = &(texts_info.key_texts[text_count]);
    GLTtext **value_text = &(texts_info.value_texts[text_count]);

    va_start(args, value);
    vsnprintf(value_buffer, sizeof(value_buffer), value, args);
    va_end(args);

    if (!*key_text) {
        *key_text = gltCreateText();
        gltSetText(*key_text, key);
    } else {
        found_max_key_width = true;
    }

    if (!*value_text) {
        *value_text = gltCreateText();
    }

    gltSetText(*value_text, value_buffer);

    if (!found_max_key_width) {
      key_width = gltGetTextWidth(*key_text, config.scale);

      if (key_width > texts_info.max_key_width) {
          texts_info.max_key_width = key_width;
      }
    }

    texts_info.text_count++;
}

static void draw_texts(void) {
    gltBeginDraw();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
    for (int i = 0; i < texts_info.text_count; i++) {
    
      GLTtext *value_text = texts_info.value_texts[i];

      if (!config.fps_only) {
        GLTtext *key_text = texts_info.key_texts[i];

        gltColor(config.key_color[0], config.key_color[1], config.key_color[2], 1.0f);
        gltDrawText2D(key_text, 10, texts_info.pos_y, config.scale);
      }

      gltColor(config.value_color[0], config.value_color[1], config.value_color[2], 1.0f);
      gltDrawText2D(value_text, texts_info.max_key_width + (texts_info.max_key_width / 2.0f), texts_info.pos_y, config.scale);

      texts_info.pos_y += gltGetLineHeight(config.scale);
    }

    gltEndDraw();
}

void draw_overlay(const char *gl_interface, unsigned int *viewport) {
    int width, height;

    width = viewport[2];
    height = viewport[3];

    if (!gltInitialized) {
       gltInit();

       prev_time = prev_time_frametime = 0;
       frames = 0;
    }

    if (prev_viewport[0] != width || prev_viewport[1] != height) {
       glViewport(viewport[0], viewport[1], width, height);
       gltViewport(width, height);
    }

    prev_viewport[0] = width;
    prev_viewport[1] = height;

    populate_overlay_info();

    
    if (config.fps_only) {
      add_text("", " %d", overlay_info.fps);
    } else {
      add_text(gl_interface, " %d FPS (%.1f ms)", overlay_info.fps, overlay_info.frametime); 
      add_text("CPU", " %d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      add_text("GPU", " %d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      add_text("VRAM", " %.2f GiB", overlay_info.gpu_mem);
      add_text("RAM", " %.2f GiB", overlay_info.mem);
    }   
    
    draw_texts();

    texts_info.text_count = 0;
    texts_info.pos_y = 10.0f;
}

void cleanup(void) {
    for (int i = 0; i < texts_info.text_count; i++) {
      GLTtext *key_text = texts_info.key_texts[i];
      GLTtext *value_text = texts_info.value_texts[i];

      gltDestroyText(key_text);
      gltDestroyText(value_text);

      key_text = NULL;
      value_text = NULL;
    }

    gltTerminate();
}
