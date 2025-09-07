#define _XOPEN_SOURCE 500

#include <time.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../include/glad.h"
#include "../include/common.h"
#include "../include/microui.h"

void gl_init(void);
void gl_flush(unsigned int *viewport, float scale);
void gl_draw_rect(mu_Rect rect, mu_Color color);
void gl_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

int gl_get_text_height(void);
int gl_get_text_width(const char *text, int len);

void populate_mem(s_overlay_info *overlay_info);
void populate_cpu(s_overlay_info *s_overlay_info);
void populate_amdgpu(s_overlay_info *overlay_info);

extern s_config config;

static int frames;

static long long prev_time;
static long long prev_time_frametime;

char vendor[128];

static mu_Context *ctx = NULL;

static s_overlay_info overlay_info = { 0 };

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

        if (strcmp(vendor, "AMD") == 0) {
           populate_amdgpu(&overlay_info);
        }
      }

      frames = 0;
      prev_time = cur_time;
    }

    prev_time_frametime = cur_time;
}

static int text_width(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return gl_get_text_width(text, len);
}

static int text_height(mu_Font font) {
  return gl_get_text_height();
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
    mu_Rect new_rect = mu_rect(5, 5, text_width(NULL, key, -1) + text_width(NULL, value_buffer, -1) + 35, 0);
      
    if ((*init_rect).w < new_rect.w) {
      (*init_rect).w = new_rect.w;
      win->rect.w = new_rect.w;
    }
    
    win->rect.h += text_height(NULL);
}

void cleanup(void) {
    // to do
}

void draw_overlay(const char *interface, unsigned int *viewport) {
  if (!ctx) {
    ctx = malloc(sizeof(mu_Context));
    mu_init(ctx);
    gl_init();

    ctx->text_width = text_width;
    ctx->text_height = text_height;
    ctx->style->colors[MU_COLOR_WINDOWBG] = mu_color(config.bg_color[0], config.bg_color[1], config.bg_color[2], config.bg_color[3]);

    strcpy(vendor, (const char*)glGetString(GL_VENDOR));

    prev_time = prev_time_frametime = 0;
    frames = 0;
  }

  populate_overlay_info();
  
  int opt = MU_OPT_NOCLOSE | MU_OPT_NOSCROLL | MU_OPT_NOTITLE | MU_OPT_NORESIZE;

  if (config.fps_only) {
    opt |= MU_OPT_NOFRAME;
  }

  mu_begin(ctx);

  mu_Rect init_rect = mu_rect(5, 5, 0, 0);

  if (mu_begin_window_ex(ctx, "", init_rect, opt)) {
    mu_Container *win = mu_get_container(ctx, "");
    win->rect.h = 0;
  
    mu_layout_row(ctx, 2, (int[]) { 50, -1 }, 0);
    if (config.fps_only) {
      add_text(ctx, &init_rect, "", "%d", overlay_info.fps);
    } else {
      add_text(ctx, &init_rect, interface, " %d FPS (%.1f ms)", overlay_info.fps, overlay_info.frametime); 
      add_text(ctx, &init_rect, "CPU", " %d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      add_text(ctx, &init_rect, "GPU", " %d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      add_text(ctx, &init_rect, "VRAM", " %.2f GiB", overlay_info.gpu_mem);
      add_text(ctx, &init_rect, "RAM", " %.2f GiB", overlay_info.mem);
    }

    win->rect.h += 5;
    
    mu_end_window(ctx);
  }

  mu_end(ctx);

  gl_flush(viewport, config.scale);

  mu_Command *cmd = NULL;
  while (mu_next_command(ctx, &cmd)) {    
    switch (cmd->type) {
        case MU_COMMAND_TEXT: gl_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: gl_draw_rect(cmd->rect.rect, cmd->rect.color); break;
    }
  }
}
