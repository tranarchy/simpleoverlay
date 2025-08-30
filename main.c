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
typedef void (*PFNGLXDESTROYCONTEXT)(Display *dpy, GLXContext ctx);

typedef EGLBoolean (*PFNEGLSWAPBUFFERS)(EGLDisplay display, EGLSurface surface);
typedef void* (*PFNEGLGETPROCADDRESS)(char const *procname);
typedef EGLBoolean (*PFNEGLTERMINATE)(EGLDisplay display);

typedef void *(*PFNDLSYM)(void*, const char*);

typedef struct s_config {
  float scale;
  
  float key_color[3];
  float value_color[3];

  bool fps_only;
} s_config;

typedef struct s_texts_info {
    GLTtext *key_texts[8];
    GLTtext *value_texts[8];

    GLfloat pos_y;
    GLfloat max_key_width;

    int text_count;
} s_texts_info;

static s_config config;
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

static unsigned int prev_viewport[2] = { 0 }; 

static drmVersionPtr drm_version = NULL;

#if defined(__FreeBSD__)
  #define dlsym_ptr dlfunc
#else
  static PFNDLSYM dlsym_ptr = NULL;
#endif

static PFNGLXSWAPBUFFERS glXSwapBuffers_ptr = NULL;
static PFNGLXGETPROCADDRESS glXGetProcAddress_ptr = NULL;
static PFNGLXGETPROCADDRESSARB glXGetProcAddressARB_ptr = NULL;
static PFNGLXDESTROYCONTEXT glXDestroyContext_ptr = NULL;

static PFNEGLSWAPBUFFERS eglSwapBuffers_ptr = NULL;
static PFNEGLGETPROCADDRESS eglGetProcAddress_ptr = NULL;
static PFNEGLTERMINATE eglTerminate_ptr = NULL;

static GLXContext glx_ctx = NULL;
static EGLContext egl_ctx = NULL;

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

static void cleanup(void) {
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

static void populate_overlay_info(void) {
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

static void add_text(const char *fmt, ...) { 
    va_list args;
    char text_buffer[256];
    GLfloat key_width;

    bool found_max_key_width = false; 
    
    int text_count = texts_info.text_count;

    GLTtext **key_text = &(texts_info.key_texts[text_count]);
    GLTtext **value_text = &(texts_info.value_texts[text_count]);

    va_start(args, fmt);
    vsnprintf(text_buffer, sizeof(text_buffer), fmt, args);
    va_end(args);

    if (!*key_text) {
        *key_text = gltCreateText();
        gltSetText(*key_text, strtok(text_buffer, "|"));
    } else {
        found_max_key_width = true;
        strtok(text_buffer, "|");
    }

    if (!*value_text) {
        *value_text = gltCreateText();
    }

    gltSetText(*value_text, strtok(NULL, "|"));

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
      GLTtext *key_text = texts_info.key_texts[i];
      GLTtext *value_text = texts_info.value_texts[i];

      gltColor(config.key_color[0], config.key_color[1], config.key_color[2], 1.0f);
      gltDrawText2D(key_text, 10, texts_info.pos_y, config.scale);

      gltColor(config.value_color[0], config.value_color[1], config.value_color[2], 1.0f);
      gltDrawText2D(value_text, texts_info.max_key_width + (texts_info.max_key_width / 2.0f), texts_info.pos_y, config.scale);

      texts_info.pos_y += gltGetLineHeight(config.scale);
    }

    gltEndDraw();
}

static void draw_overlay(unsigned int *viewport) {
    int width, height;

    width = viewport[2];
    height = viewport[3];

    if (!gltInitialized) {
       gltInit();
    }

    if (prev_viewport[0] != width || prev_viewport[1] != height) {
       glViewport(viewport[0], viewport[1], width, height);
       gltViewport(width, height);
    }

    prev_viewport[0] = width;
    prev_viewport[1] = height;

    populate_overlay_info();
    
    add_text("FPS |%d (%.1f ms)", overlay_info.fps, overlay_info.frametime);
    
    if (!config.fps_only) {
      add_text("CPU |%d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      add_text("GPU |%d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      add_text("VRAM |%.2f GiB", overlay_info.gpu_mem);
      add_text("RAM |%.2f GiB", overlay_info.mem);
    }   
    
    draw_texts();

    texts_info.text_count = 0;
    texts_info.pos_y = 10.0f;
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surf) {
    if (!eglSwapBuffers_ptr) {
      eglSwapBuffers_ptr = (PFNEGLSWAPBUFFERS)dlsym_ptr(RTLD_NEXT, "eglSwapBuffers");
    }
    
    EGLContext old_ctx = eglGetCurrentContext();

    unsigned int viewport[4];
    glGetIntegerv(GL_VIEWPORT, (int*)viewport);
    
    eglQuerySurface(display, surf, EGL_WIDTH, (EGLint*)&viewport[2]);
    eglQuerySurface(display, surf, EGL_HEIGHT, (EGLint*)&viewport[3]);

    eglMakeCurrent(display, surf, surf, get_egl_ctx(display));
    
    draw_overlay(viewport);
    
    eglMakeCurrent(display, surf, surf, old_ctx);
    
    return eglSwapBuffers_ptr(display, surf);
}

void (*eglGetProcAddress(char const *procname))(void) {
    if (strcmp(procname, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } else if (strcmp(procname, "eglTerminate") == 0) {
        return (void*)eglTerminate;
    }

    if (!eglGetProcAddress_ptr) {
       eglGetProcAddress_ptr = (PFNEGLGETPROCADDRESS)dlsym_ptr(RTLD_NEXT, "eglGetProcAddress"); 
    }
          
    return eglGetProcAddress_ptr(procname);
}

EGLBoolean eglTerminate(EGLDisplay display) {
    if (!eglTerminate_ptr) {
       eglTerminate_ptr = (PFNEGLTERMINATE)dlsym_ptr(RTLD_NEXT, "eglTerminate"); 
    }

    eglDestroyContext(display, egl_ctx);
    egl_ctx = NULL;
    cleanup();

    return eglTerminate_ptr(display);
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
    if (!glXSwapBuffers_ptr) {
      glXSwapBuffers_ptr = (PFNGLXSWAPBUFFERS)dlsym_ptr(RTLD_NEXT, "glXSwapBuffers");
    }

    GLXContext old_ctx = glXGetCurrentContext();

    unsigned int viewport[4];
    glGetIntegerv(GL_VIEWPORT, (int*)viewport);
    
    glXQueryDrawable(dpy, drawable, GLX_WIDTH, &viewport[2]);
    glXQueryDrawable(dpy, drawable, GLX_HEIGHT, &viewport[3]);
    
    glXMakeCurrent(dpy, drawable, get_glx_ctx(dpy));
    
    draw_overlay(viewport);
  
    glXMakeCurrent(dpy, drawable, old_ctx);
    glXSwapBuffers_ptr(dpy, drawable);
}

void (*glXGetProcAddress(const GLubyte *procName))(void) {
    if (strcmp((const char *)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp((const char *)procName, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    }

    if (!glXGetProcAddress_ptr) {
      glXGetProcAddress_ptr = (PFNGLXGETPROCADDRESS)dlsym_ptr(RTLD_NEXT, "glXGetProcAddress");
    }
    
    return glXGetProcAddress_ptr(procName);
}

void (*glXGetProcAddressARB(const GLubyte *procName))(void) {
    if (strcmp((const char *)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp((const char *)procName, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    }

    if (!glXGetProcAddressARB_ptr) {
      glXGetProcAddressARB_ptr = (PFNGLXGETPROCADDRESSARB)dlsym_ptr(RTLD_NEXT, "glXGetProcAddressARB");
    }
    
    return glXGetProcAddressARB_ptr(procName);
}

void glXDestroyContext(Display *dpy, GLXContext ctx) {
    if (!glXDestroyContext_ptr) {
       glXDestroyContext_ptr = (PFNGLXDESTROYCONTEXT)dlsym_ptr(RTLD_NEXT, "glXDestroyContext"); 
    }

    cleanup();
    
    glXDestroyContext_ptr(dpy, ctx);
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
    } else if (strcmp(symbol, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    } else if (strcmp(symbol, "eglGetProcAddress") == 0) {
        return (void*)eglGetProcAddress;
    } else if (strcmp(symbol, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } else if (strcmp(symbol, "eglTerminate") == 0) {
        return (void*)eglTerminate;
    }

    return dlsym_ptr(handle, symbol);
}

