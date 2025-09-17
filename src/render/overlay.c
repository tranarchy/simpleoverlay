#define _XOPEN_SOURCE 500

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <glad.h>
#include <common.h>
#include <microui.h>

typedef void (*PFNGLINIT)(void);
typedef void (*PFNGLFLUSH)(mu_Rect rect, unsigned int* viewport);
typedef void (*PFNGLDRAWRECT)(mu_Rect rect, mu_Color color);
typedef void (*PFNGLDRAWTEXT)(const char *text, mu_Vec2 pos, mu_Color color);
typedef int (*PFNGLGETTEXTHEIGHT)(void);
typedef int (*PFNGLGETTEXTWIDTH)(const char *text, int len);

void gl1_init(void);
void gl1_flush(mu_Rect rect, unsigned int* viewport);
void gl1_draw_rect(mu_Rect rect, mu_Color color);
void gl1_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

int gl1_get_text_height(void);
int gl1_get_text_width(const char *text, int len);

void gl3_init(void);
void gl3_flush(mu_Rect rect, unsigned int* viewport);
void gl3_draw_rect(mu_Rect rect, mu_Color color);
void gl3_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

int gl3_get_text_height(void);
int gl3_get_text_width(const char *text, int len);

void populate_mem(s_overlay_info *overlay_info);
void populate_cpu(s_overlay_info *s_overlay_info);

#ifndef __APPLE__
  void populate_amdgpu(s_overlay_info *overlay_info);
  void populate_nvidia(s_overlay_info *overlay_info);
#else
  void populate_applegpu(s_overlay_info *overlay_info);
#endif

static PFNGLINIT gl_init_ptr = NULL;
static PFNGLFLUSH gl_flush_ptr = NULL;
static PFNGLDRAWRECT gl_draw_rect_ptr = NULL;
static PFNGLDRAWTEXT gl_draw_text_ptr = NULL;
static PFNGLGETTEXTHEIGHT gl_get_text_height_ptr = NULL;
static PFNGLGETTEXTWIDTH gl_get_text_width_ptr = NULL;

extern s_config config;

typedef enum GPU_DRIVER {
   AMDGPU,
   APPLE,
   NVIDIA,
   UNKNOWN
} GPU_DRIVER;

static int frames;

static long long prev_time;
static long long prev_time_frametime;

static GPU_DRIVER gpu_driver;

static float frametimes[100] = { 0 };

static mu_Context *ctx = NULL;

static s_overlay_info overlay_info = { 0 };

static long long get_time_nano(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1e9 + ts.tv_nsec;
}

static void populate_overlay_info(void) {
    long long cur_time = get_time_nano();

    float frametime = (cur_time - prev_time_frametime) / 1e6f;

    for (int i = 0; i < 99; i++) {
        frametimes[i] = frametimes[i + 1];
    }

    frametimes[99] = frametime;

    frames++;

    if (cur_time - prev_time >= 1e9) {
      overlay_info.fps = frames;
      overlay_info.frametime = frametime;

      if (!config.fps_only) {
        populate_cpu(&overlay_info);
        populate_mem(&overlay_info);
        
        #ifndef __APPLE__
          if (gpu_driver == AMDGPU) {
            populate_amdgpu(&overlay_info);
          } else if (gpu_driver == NVIDIA) {
            populate_nvidia(&overlay_info);
          }
        #else
          if (gpu_driver == APPLE) {
            populate_applegpu(&overlay_info);
          }
        #endif
      }

      frames = 0;
      prev_time = cur_time;
    }

    prev_time_frametime = cur_time;
}

static int text_width(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return gl_get_text_width_ptr(text, len);
}

static int text_height(mu_Font font) {
  return gl_get_text_height_ptr();
}

static void add_text(mu_Context *ctx, mu_Rect *init_rect, const char *key, const char *value, ...) { 
    va_list args;
    char value_buffer[256];

    va_start(args, value);
    vsprintf(value_buffer, value, args);
    va_end(args);

    if (!config.fps_only) {
      ctx->style->colors[MU_COLOR_TEXT] = mu_color(config.key_color[0], config.key_color[1], config.key_color[2], config.key_color[3]);
      mu_text(ctx, key);
    }

    ctx->style->colors[MU_COLOR_TEXT] = mu_color(config.value_color[0], config.value_color[1], config.value_color[2], config.value_color[3]);
    mu_text(ctx, value_buffer);

    mu_Container *win = mu_get_container(ctx, "");
    mu_Rect new_rect;
    
    if (!config.fps_only) {
      new_rect = mu_rect(5, 5, text_width(NULL, key, -1) + text_width(NULL, value_buffer, -1) + 35, 0);
    } else {
      new_rect = mu_rect(5, 5, text_width(NULL, value_buffer, -1) + 5, 0);
    }
    
      
    if ((*init_rect).w < new_rect.w) {
      (*init_rect).w = new_rect.w;
      win->rect.w = new_rect.w;
    }
    
    win->rect.h += text_height(NULL);
}

static void draw_graph(mu_Container *win) {
    mu_Rect frametime_r_base = mu_layout_next(ctx);
    frametime_r_base.y += 15;
    frametime_r_base.w =  win->rect.w - 5;
    frametime_r_base.h = 1;

    int graph_max_height = 10;
      
    float frametime_sum = 0;
    float frametime_avg = 0;

    for (int i = 1; i < 100; i++) {
        float frametime = frametimes[i];

        if (frametime == 0) {
          break;
        }

        frametime_sum += frametime;
    }

    frametime_avg = frametime_sum / 100;

    int frametime_element_width = (int)round((float)win->rect.w / 100.0f);
    
    for (int i = 0; i < 100; i++) {
        float frametime = frametimes[i];

        if (frametime == 0) {
          break;
        }

        float difference = frametime - frametime_avg;
        float percent = (difference / frametime_avg) * 100.0f;

        if (percent < 1.0f && percent > -1.0f) {
          continue;
        }

        if (percent < -50) {
          percent = -50;
        } else if (percent > 50) {
          percent = 50;
        }

        int bar_height = (int)round(abs(percent) / 50.0f * (float)graph_max_height);

        if (bar_height > graph_max_height) {
          bar_height = graph_max_height;
        }

        mu_Rect frametime_r = mu_rect(0, 0, 0, 0);
        frametime_r.x = frametime_r_base.x + (i * frametime_element_width);
        frametime_r.w = frametime_element_width;
        frametime_r.h = bar_height;

        if (frametime_r.h > graph_max_height) {
          frametime_r.h = graph_max_height;
        }

        if (frametime_avg < frametime) {
          frametime_r.y = frametime_r_base.y - frametime_r.h + 1;
        } else if (frametime_avg > frametime) {
          frametime_r.y = frametime_r_base.y + 1;
        } else {
          frametime_r.y = frametime_r_base.y;
          frametime_r.h = 1;
        }
 
        mu_draw_rect(ctx, frametime_r, mu_color(config.key_color[0], config.key_color[1], config.key_color[2], config.key_color[3]));
      }

      mu_draw_rect(ctx, frametime_r_base, mu_color(config.key_color[0], config.key_color[1], config.key_color[2], config.key_color[3]));
}

void draw_overlay(const char *interface, unsigned int *viewport) {
  if (!ctx) {
    ctx = malloc(sizeof(mu_Context));
    mu_init(ctx);

    int gl_major_version = -1;
    glGetIntegerv(GL_MAJOR_VERSION, &gl_major_version);

    if (gl_major_version < 3) {
      gl_init_ptr = &gl1_init;
      gl_flush_ptr = &gl1_flush;
      gl_draw_rect_ptr = &gl1_draw_rect;
      gl_draw_text_ptr = &gl1_draw_text;
      gl_get_text_height_ptr = &gl1_get_text_height;
      gl_get_text_width_ptr = &gl1_get_text_width;
    } else {
      gl_init_ptr = &gl3_init;
      gl_flush_ptr = &gl3_flush;
      gl_draw_rect_ptr = &gl3_draw_rect;
      gl_draw_text_ptr = &gl3_draw_text;
      gl_get_text_height_ptr = &gl3_get_text_height;
      gl_get_text_width_ptr = &gl3_get_text_width;
    }

    gl_init_ptr();
   
    ctx->text_width = text_width;
    ctx->text_height = text_height;
    ctx->style->colors[MU_COLOR_WINDOWBG] = mu_color(config.bg_color[0], config.bg_color[1], config.bg_color[2], config.bg_color[3]);

    const char *vendor = (const char*)glGetString(GL_VENDOR);

    if (strcmp(vendor, "AMD") == 0) {
      gpu_driver = AMDGPU;
    } else if (strcmp(vendor, "NVIDIA Corporation") == 0) {
      gpu_driver = NVIDIA;
    } else if (strcmp(vendor, "Apple") == 0) {
      gpu_driver = APPLE;
    } else {
      gpu_driver = UNKNOWN;
    }

    prev_time = prev_time_frametime = 0;
    frames = 0;
  }

  populate_overlay_info();
  
  int opt = MU_OPT_NOCLOSE | MU_OPT_NOSCROLL | MU_OPT_NOTITLE | MU_OPT_NORESIZE;

  if (config.fps_only) {
    opt |= MU_OPT_NOFRAME;
  }

  mu_begin(ctx);
  
  mu_Container *win;
  mu_Rect init_rect = mu_rect(0, 0, 0, 0);

  if (mu_begin_window_ex(ctx, "", init_rect, opt)) {
    win = mu_get_container(ctx, "");
    win->rect.h = 0;

    mu_layout_row(ctx, 2, (int[]) { 50, -1 }, 0);
    if (config.fps_only) {
      add_text(ctx, &init_rect, "", "%d", overlay_info.fps);
    } else {
      if ((config.metrics >> 4) & 1) {
        add_text(ctx, &init_rect, interface, " %d FPS (%.1f ms)", overlay_info.fps, overlay_info.frametime); 
      }
      
      if ((config.metrics >> 3) & 1) {
        add_text(ctx, &init_rect, "CPU", " %d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      }
      
      if ((config.metrics >> 2) & 1) {
        add_text(ctx, &init_rect, "GPU", " %d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      }

      if ((config.metrics >> 1) & 1) {
        add_text(ctx, &init_rect, "VRAM", " %.2f GiB", overlay_info.gpu_mem);
      }

      if (config.metrics & 1) {
        add_text(ctx, &init_rect, "RAM", " %.2f GiB", overlay_info.mem);
      }
    }

    win->rect.h += 5;

    if (!config.no_graph && !config.fps_only) {
      win->rect.h += 30;
      draw_graph(win);
    }

    mu_end_window(ctx);
  }

  mu_end(ctx);
  
  mu_Command *cmd = NULL;
  while (mu_next_command(ctx, &cmd)) {    
    switch (cmd->type) {
        case MU_COMMAND_TEXT: gl_draw_text_ptr(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: gl_draw_rect_ptr(cmd->rect.rect, cmd->rect.color); break;
    }
  }

  gl_flush_ptr(win->rect, viewport);
}
