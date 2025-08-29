#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT

#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <GL/glx.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "xf86drm.h"

#include "include/gltext.h"
#include "include/overlay.h"
#include "include/elfhacks.h"

void populate_mem(s_overlay_info *overlay_info);
void populate_cpu(s_overlay_info *s_overlay_info);
void populate_amdgpu(s_overlay_info *overlay_info);

typedef void (*PFNGLXSWAPBUFFERS)(Display *dpy, GLXDrawable drawable);
typedef void* (*PFNGLXGETPROCADDRESS)(const GLubyte *procName);
typedef void* (*PFNGLXGETPROCADDRESSARB)(const GLubyte *procName);

typedef EGLBoolean (*PFNEGLSWAPBUFFERS)(EGLDisplay display, EGLSurface surface);
typedef void* (*PFNEGLGETPROCADDRESS)(char const *procname);

typedef void *(*PFNDLSYM)(void*, const char*);

typedef struct s_config {
  float scale;
  
  float key_color[3];
  float value_color[3];

  bool fps_only;
} s_config;

typedef struct s_texts_info {
    GLfloat max_value_width;
    GLfloat max_key_width;
     
    GLfloat full_text_height;

    int text_count;
} s_texts_info;

static int frames;
static int text_pos_y;

static long long prev_time;
static long long prev_time_frametime;

static drmVersionPtr drm_version = NULL;

#if defined(__FreeBSD__)
  #define dlsym_ptr dlfunc
#else
  static PFNDLSYM dlsym_ptr = NULL;
#endif

static GLXContext glx_ctx = NULL;
static EGLContext egl_ctx = NULL;

static s_config config;
static s_overlay_info overlay_info = { 0 };

static GLint old_viewport[2] = { 0 }; 

static void hex_to_rgb(char *hex, float *rgb) {
  long hex_num = strtol(hex, NULL, 16);

  rgb[0] = ((hex_num >> 16) & 0xFF) / 255.0f;
  rgb[1] = ((hex_num >> 8) & 0xFF) / 255.0f;
  rgb[2] = (hex_num & 0xFF) / 255.0f;
}

static long long get_time_nano(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1e9 + ts.tv_nsec;
}

static void get_drm_version(void) {
  int fd = open("/dev/dri/renderD128", O_RDONLY);
  drm_version = drmGetVersion(fd);
  close(fd);
}

__attribute__((constructor))
static void init(void) {
  config.scale = getenv("SCALE") ? atof(getenv("SCALE")) : 1.8f;
  hex_to_rgb(getenv("KEY_COLOR") ? getenv("KEY_COLOR") : "9889FA", config.key_color);
  hex_to_rgb(getenv("VALUE_COLOR") ? getenv("VALUE_COLOR") : "FFFFFF", config.value_color);
  config.fps_only = getenv("FPS_ONLY") ? atoi(getenv("FPS_ONLY")) : false;

  get_drm_version();

  prev_time = prev_time_frametime = get_time_nano();
  frames = 0;
  text_pos_y = 10;
}

static GLXContext get_glx_ctx(Display *dpy) {
    if (glx_ctx) {
      return glx_ctx;
    }

    const int visual_attribs[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER    , True,
        None
    };

    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attribs, &fbcount);
    
    glx_ctx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, 0, True);

    XFree(fbc);

    return glx_ctx;
}

static EGLContext get_egl_ctx(EGLDisplay display) {
     if (egl_ctx) {
        return egl_ctx;
     }

    egl_ctx = eglCreateContext(display, NULL, EGL_NO_CONTEXT, NULL);

    return egl_ctx;
}

static void populate_overlay_info() {
    long long cur_time = get_time_nano();

    frames++;

    if (cur_time - prev_time >= 1e9) {
      overlay_info.fps = frames;
      overlay_info.frametime = (cur_time - prev_time_frametime) / 1e6f;

      populate_cpu(&overlay_info);
      populate_mem(&overlay_info);

      if (drm_version != NULL) {
        if (strcmp(drm_version->name, "amdgpu") == 0) {
          populate_amdgpu(&overlay_info);
        }
      }

      frames = 0;
      prev_time = cur_time;
    }

    prev_time_frametime = cur_time;
}

static void add_text(char (*texts)[256], s_texts_info *texts_info, const char *fmt, ...) { 
    va_list args;
    char buffer[256];

    int text_cunt = texts_info->text_count;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    strlcpy(texts[text_cunt], buffer, sizeof(texts[text_cunt]));

    GLTtext *text_key = gltCreateText();
    gltSetText(text_key, strtok(buffer, "|"));

    GLTtext *text_value = gltCreateText();
    gltSetText(text_value, strtok(NULL, "|"));

    GLfloat key_width = gltGetTextWidth(text_key, config.scale);
    GLfloat value_width = gltGetTextWidth(text_value, config.scale);

    texts_info->full_text_height += gltGetTextHeight(text_key, config.scale);

    gltDestroyText(text_key);
    gltDestroyText(text_value);

    if (value_width > texts_info->max_value_width) {
        texts_info->max_value_width = value_width;
    }

    if (key_width > texts_info->max_key_width) {
        texts_info->max_key_width = key_width;
    }

    texts_info->text_count++;
}

static void draw_texts(char (*texts)[256], s_texts_info texts_info) {
    GLTtext *key_text;
    GLTtext *value_text;

    gltBeginDraw();
 
    for (int i = 0; i < texts_info.text_count; i++) {
      char *key = strtok(texts[i], "|");
      char *value = strtok(NULL, "|");

      key_text = gltCreateText();
      value_text = gltCreateText();

      gltSetText(key_text, key);
      gltSetText(value_text, value);

      gltColor(config.key_color[0], config.key_color[1], config.key_color[2], 1.0f);
      gltDrawText2D(key_text, 10, text_pos_y, config.scale);

      gltColor(config.value_color[0], config.value_color[1], config.value_color[2], 1.0f);
      gltDrawText2D(value_text, texts_info.max_key_width + (texts_info.max_key_width / 2.0f), text_pos_y, config.scale);

      gltDestroyText(key_text);
      gltDestroyText(value_text);

      text_pos_y += gltGetLineHeight(config.scale);
    }

    gltEndDraw();
    gltTerminate();
}

static void draw_bg(int width, int height, s_texts_info texts_info) {
    GLfloat bg_height = texts_info.full_text_height;
    GLfloat bg_width = texts_info.max_key_width + (texts_info.max_key_width / 2.0f) + texts_info.max_value_width;

    glColor4f(0.06274f, 0.06274f, 0.10196f, 0.78431f);
    glBegin(GL_QUADS);
      glVertex2f(5, height - 10);
      glVertex2f(5 + bg_width, height - 10);
      glVertex2f(5 + bg_width, height - bg_height - 10);
      glVertex2f(5, height - bg_height - 10);
    glEnd();
}

static void draw_overlay(GLint *viewport) {
    int width, height;
    char texts[64][256];
    
    s_texts_info texts_info = { 0 };

    width = viewport[2];
    height = viewport[3];

    if (old_viewport[0] != width || old_viewport[1] != height) {
      glViewport(viewport[0], viewport[1], width, height);
      gltViewport(width, height);
    }

    old_viewport[0] = width;
    old_viewport[1] = height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    populate_overlay_info();
    
    add_text(texts, &texts_info, "FPS |%d (%.1f ms)", overlay_info.fps, overlay_info.frametime);
    
    if (!config.fps_only) {
      add_text(texts, &texts_info, "CPU |%d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      add_text(texts, &texts_info, "GPU |%d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      add_text(texts, &texts_info, "VRAM |%.2f GiB", overlay_info.gpu_mem);
      add_text(texts, &texts_info, "RAM |%.2f GiB", overlay_info.mem);
    }

    
     gltInit();
    draw_bg(width, height, texts_info);
    draw_texts(texts, texts_info);

    text_pos_y = 10;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surf) {
    PFNEGLSWAPBUFFERS eglSwapBuffers_ptr = (PFNEGLSWAPBUFFERS)dlsym_ptr(RTLD_NEXT, "eglSwapBuffers");
    EGLContext old_ctx = eglGetCurrentContext();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    eglMakeCurrent(display, surf, surf, get_egl_ctx(display));
    
    draw_overlay(viewport);
    
    eglMakeCurrent(display, surf, surf, old_ctx);
    
    return eglSwapBuffers_ptr(display, surf);
}

void (*eglGetProcAddress(char const *procname))(void) {
    if (strcmp(procname, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    }
    
    PFNEGLGETPROCADDRESS eglGetProcAddress_ptr = (PFNEGLGETPROCADDRESS)dlsym_ptr(RTLD_NEXT, "eglGetProcAddress");        
    return eglGetProcAddress_ptr(procname);
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
    PFNGLXSWAPBUFFERS glXSwapBuffers_ptr = (PFNGLXSWAPBUFFERS)dlsym_ptr(RTLD_NEXT, "glXSwapBuffers");
    GLXContext old_ctx = glXGetCurrentContext();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    glXMakeCurrent(dpy, drawable, get_glx_ctx(dpy));
    
    draw_overlay(viewport);
  
    glXMakeCurrent(dpy, drawable, old_ctx);
    glXSwapBuffers_ptr(dpy, drawable);
}

void (*glXGetProcAddress(const GLubyte *procName))(void) {
    if (strcmp((const char *)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    }
    
    PFNGLXGETPROCADDRESS glXGetProcAddress_ptr = (PFNGLXGETPROCADDRESS)dlsym_ptr(RTLD_NEXT, "glXGetProcAddress");        
    return glXGetProcAddress_ptr(procName);
}

void (*glXGetProcAddressARB(const GLubyte *procName))(void) {
    if (strcmp((const char *)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    }
    
    PFNGLXGETPROCADDRESSARB glXGetProcAddressARB_ptr = (PFNGLXGETPROCADDRESSARB)dlsym_ptr(RTLD_NEXT, "glXGetProcAddressARB");
    return glXGetProcAddressARB_ptr(procName);
}

void* dlsym(void *handle, const char *symbol) {
     if (!dlsym_ptr) {
        eh_obj_t libdl;
        eh_find_obj(&libdl, "*libc.so*");
        eh_find_sym(&libdl, "dlsym", (void **) &dlsym_ptr);
     }

    if (strcmp(symbol, "glXGetProcAddress") == 0) {
        return (void*)glXGetProcAddress;
    } else if (strcmp(symbol, "glXGetProcAddressARB") == 0) {
        return (void*)glXGetProcAddressARB;
    } else if (strcmp(symbol, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp(symbol, "eglGetProcAddress") == 0) {
        return (void*)eglGetProcAddress;
    } else if (strcmp(symbol, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } 

    return dlsym_ptr(handle, symbol);
}

