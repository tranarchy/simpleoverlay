#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT

#include <time.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "include/glad.h"

#include "include/gltext.h"
#include "include/overlay.h"
#include "include/elfhacks.h"

#if defined(__OpenBSD__)
  #define GL_LIB "libGL.so"
  #define EGL_LIB "libEGL.so"
#else
  #define GL_LIB "libGL.so.1"
  #define EGL_LIB "libEGL.so.1"
#endif

void populate_mem(s_overlay_info *overlay_info);
void populate_cpu(s_overlay_info *s_overlay_info);
void populate_amdgpu(s_overlay_info *overlay_info);

typedef void *(*PFNDLSYM)(void *handle, const char *symbol);

typedef int (*PFNXDEFAULTSCREEN)(void *display);

typedef void (*PFNGLXSWAPBUFFERS)(void *dpy, void *drawable);
typedef void *(*PFNGLXGETPROCADDRESS)(const unsigned char *procName);
typedef void *(*PFNGLXGETPROCADDRESSARB)(const unsigned char *procName);
typedef void (*PFNGLXDESTROYCONTEXT)(void *dpy, void *ctx);
typedef void *(*PFNGLXCHOOSEFBCONFIG)(void *dpy, int screen, const int *attrib_list, int *nelements);
typedef void *(*PFNGLXCREATENEWCONTEXT)(void *dpy, void *config, int render_type, void *share_list, int direct);
typedef void *(*PFNGLXGETCURRENTCONTEXT)(void);
typedef int (*PFNGLXQUERYDRAWABLE)(void *dpy, void *draw, int attribute, unsigned int *value);
typedef int (*PFNGLXMAKECURRENT)(void *dpy, void *drawable, void *ctx);

typedef unsigned int (*PFNEGLSWAPBUFFERS)(void *display, void *surface);
typedef void *(*PFNEGLGETPROCADDRESS)(const char *procname);
typedef unsigned int (*PFNEGLTERMINATE)(void *display);
typedef void *(*PFNEGLGETCURRENTCONTEXT)(void);
typedef unsigned int (*PFNEGLMAKECURRENT)(void *display, void *draw, void *read, void *context);
typedef unsigned int (*PFNEGLQUERYSURFACE)(void *display, void *surface, int attribute, int *value);
typedef void *(*PFNEGLCREATECONTEXT)(void *display, void *config, void* share_context, const int *attrib_list);
typedef unsigned int (*PFNEGLDESTROYCONTEXT)(void *display, void *context);

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

#if defined(__FreeBSD__)
  #define dlsym_ptr dlfunc
#else
  static PFNDLSYM dlsym_ptr = NULL;
#endif

static PFNGLXSWAPBUFFERS glXSwapBuffers_ptr = NULL;
static PFNGLXGETPROCADDRESS glXGetProcAddress_ptr = NULL;
static PFNGLXGETPROCADDRESSARB glXGetProcAddressARB_ptr = NULL;
static PFNGLXDESTROYCONTEXT glXDestroyContext_ptr = NULL;
static PFNGLXGETCURRENTCONTEXT glXGetCurrentContext_ptr = NULL;
static PFNGLXQUERYDRAWABLE glXQueryDrawable_ptr = NULL;
static PFNGLXMAKECURRENT glXMakeCurrent_ptr = NULL;

static PFNEGLSWAPBUFFERS eglSwapBuffers_ptr = NULL;
static PFNEGLGETPROCADDRESS eglGetProcAddress_ptr = NULL;
static PFNEGLTERMINATE eglTerminate_ptr = NULL;
static PFNEGLGETCURRENTCONTEXT eglGetCurrentContext_ptr = NULL;
static PFNEGLMAKECURRENT eglMakeCurrent_ptr = NULL;
static PFNEGLQUERYSURFACE eglQuerySurface_ptr = NULL;
static PFNEGLDESTROYCONTEXT eglDestroyContext_ptr = NULL;

static void *gl_handle, *egl_handle = NULL;

static void *glx_ctx, *prev_glx_ctx = NULL;
static void *egl_ctx, *prev_egl_ctx = NULL;

static void hex_to_rgb(char *hex, float *rgb) {
  long hex_num = strtol(hex, NULL, 16);

  rgb[0] = ((hex_num >> 16) & 0xFF) / 255.0f;
  rgb[1] = ((hex_num >> 8) & 0xFF) / 255.0f;
  rgb[2] = (hex_num & 0xFF) / 255.0f;
}

__attribute__((constructor))
static void init(void) {
  config.scale = getenv("SCALE") ? atof(getenv("SCALE")) : 1.8f;
  hex_to_rgb(getenv("KEY_COLOR") ? getenv("KEY_COLOR") : "9889FA", config.key_color);
  hex_to_rgb(getenv("VALUE_COLOR") ? getenv("VALUE_COLOR") : "FFFFFF", config.value_color);
  config.fps_only = getenv("FPS_ONLY") ? atoi(getenv("FPS_ONLY")) : false;

  prev_time = prev_time_frametime = 0;
  frames = 0;
}

void *get_libgl_addr(const char *proc_name) {
  if (!gl_handle) {
    gl_handle = dlopen(GL_LIB, RTLD_LAZY);
  }

  return dlsym_ptr(gl_handle, proc_name);
}

void *get_libegl_addr(const char *proc_name) {
  if (!egl_handle) {
    egl_handle = dlopen(EGL_LIB, RTLD_LAZY);
  }

  return dlsym_ptr(egl_handle, proc_name);
}

static void *get_glx_ctx(void *dpy) {
    if (glx_ctx) {
      return glx_ctx;
    }

    int visual_attribs[] = {
        0x8012, true,
        0x8010, 0x00000001,
        0x8011, 0x00000001,
        0x22, 0x8002,
        5, true,
        0x8000
    };

    void *x11_handle = dlopen("libX11.so", RTLD_LAZY);
    
    PFNXDEFAULTSCREEN XDefaultScreen_ptr = (PFNXDEFAULTSCREEN)dlsym_ptr(x11_handle, "XDefaultScreen");
    PFNGLXCHOOSEFBCONFIG glXChooseFBConfig_ptr = (PFNGLXCHOOSEFBCONFIG)get_libgl_addr("glXChooseFBConfig"); 
    PFNGLXCREATENEWCONTEXT glXCreateNewContext_ptr = (PFNGLXCREATENEWCONTEXT)get_libgl_addr("glXCreateNewContext");

    int fbcount;
    void** fbc = glXChooseFBConfig_ptr(dpy, XDefaultScreen_ptr(dpy), visual_attribs, &fbcount);
    
    glx_ctx = glXCreateNewContext_ptr(dpy, fbc[0], 0x8014, NULL, true);

    return glx_ctx;
}

static void *get_egl_ctx(void *display) {
     if (egl_ctx) {
        return egl_ctx;
     }

    PFNEGLCREATECONTEXT eglCreateContext_ptr = (PFNEGLCREATECONTEXT)get_libegl_addr("eglCreateContext");

    egl_ctx = eglCreateContext_ptr(display, NULL, (void*)0, NULL);

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
    
    add_text( glx_ctx ? "GLX" : "EGL", " %d FPS (%.1f ms)", overlay_info.fps, overlay_info.frametime);
    
    if (!config.fps_only) {
      add_text("CPU", " %d%% (%d C)", overlay_info.cpu_usage, overlay_info.cpu_temp);
      add_text("GPU", " %d%% (%d C)", overlay_info.gpu_usage, overlay_info.gpu_temp);
      add_text("VRAM", " %.2f GiB", overlay_info.gpu_mem);
      add_text("RAM", " %.2f GiB", overlay_info.mem);
    }   
    
    draw_texts();

    texts_info.text_count = 0;
    texts_info.pos_y = 10.0f;
}

unsigned int eglSwapBuffers(void *display, void *surf) {
    if (!eglSwapBuffers_ptr) {
      gladLoadGL();
      
      eglSwapBuffers_ptr = (PFNEGLSWAPBUFFERS)get_libegl_addr("eglSwapBuffers");
      eglMakeCurrent_ptr = (PFNEGLMAKECURRENT)get_libegl_addr("eglMakeCurrent");
      eglQuerySurface_ptr = (PFNEGLQUERYSURFACE)get_libegl_addr("eglQuerySurface");
      eglGetCurrentContext_ptr = (PFNEGLGETCURRENTCONTEXT)get_libegl_addr("eglGetCurrentContext");
    }

    if (!prev_egl_ctx) {
      prev_egl_ctx = eglGetCurrentContext_ptr();
    }

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    eglQuerySurface_ptr(display, surf, 0x3057, &viewport[2]);
    eglQuerySurface_ptr(display, surf, 0x3056, &viewport[3]);

    eglMakeCurrent_ptr(display, surf, surf, get_egl_ctx(display));
    
    draw_overlay((unsigned int*)viewport);
    
    eglMakeCurrent_ptr(display, surf, surf, prev_egl_ctx);
    
    return eglSwapBuffers_ptr(display, surf);
}

unsigned int eglTerminate(void *display) {
    if (!eglTerminate_ptr) {
       eglTerminate_ptr = (PFNEGLTERMINATE)get_libegl_addr("eglTerminate"); 
       eglDestroyContext_ptr = (PFNEGLDESTROYCONTEXT)get_libegl_addr("eglDestroyContext"); 
    }

    eglDestroyContext_ptr(display, egl_ctx);
    egl_ctx = NULL;
    cleanup();

    return eglTerminate_ptr(display);
}

void (*eglGetProcAddress(char const *procname))(void) {
    if (strcmp(procname, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } else if (strcmp(procname, "eglTerminate") == 0) {
        return (void*)eglTerminate;
    }

    if (!eglGetProcAddress_ptr) {
      eglGetProcAddress_ptr = (PFNEGLGETPROCADDRESS)get_libegl_addr("eglGetProcAddress");
    }
          
    return eglGetProcAddress_ptr(procname);
}

void glXSwapBuffers(void *dpy, void *drawable) {
    if (!glXSwapBuffers_ptr) {
      gladLoadGL();
    
      glXSwapBuffers_ptr = (PFNGLXSWAPBUFFERS)get_libgl_addr("glXSwapBuffers");
      glXGetCurrentContext_ptr = (PFNGLXGETCURRENTCONTEXT)get_libgl_addr("glXGetCurrentContext");
      glXQueryDrawable_ptr = (PFNGLXQUERYDRAWABLE)get_libgl_addr("glXQueryDrawable");
      glXMakeCurrent_ptr = (PFNGLXMAKECURRENT)get_libgl_addr("glXMakeCurrent");
    } 

    if (!prev_glx_ctx) {
        prev_glx_ctx = glXGetCurrentContext_ptr();
    }

    unsigned int viewport[4];
    glGetIntegerv(GL_VIEWPORT, (int*)viewport);
    
    glXQueryDrawable_ptr(dpy, drawable, 0x801D, &viewport[2]);
    glXQueryDrawable_ptr(dpy, drawable, 0x801E, &viewport[3]);
    
    glXMakeCurrent_ptr(dpy, drawable, get_glx_ctx(dpy));
    
    draw_overlay(viewport);
  
    glXMakeCurrent_ptr(dpy, drawable, prev_glx_ctx);
    glXSwapBuffers_ptr(dpy, drawable);
}

void glXDestroyContext(void *dpy, void *ctx) {
    if (!glXDestroyContext_ptr) {
       glXDestroyContext_ptr = (PFNGLXDESTROYCONTEXT)get_libgl_addr("glXDestroyContext"); 
    }    

    cleanup();
    
    glXDestroyContext_ptr(dpy, ctx);
}

void *glXGetProcAddress(const unsigned char *procName) {
    if (strcmp((const char*)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp((const char*)procName, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    }

    if (!glXGetProcAddress_ptr) {
      glXGetProcAddress_ptr = (PFNGLXGETPROCADDRESS)get_libgl_addr("glXGetProcAddress");
    }
    
    return glXGetProcAddress_ptr(procName);
}

void *glXGetProcAddressARB(const unsigned char *procName) {
    if (strcmp((const char*)procName, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp((const char*)procName, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    }
    
    if (!glXGetProcAddressARB_ptr) {
      glXGetProcAddressARB_ptr = (PFNGLXGETPROCADDRESSARB)get_libgl_addr("glXGetProcAddressARB");
    }
    
    return glXGetProcAddressARB_ptr(procName);
}

void* dlsym(void *handle, const char *symbol) {
    if (!dlsym_ptr) {
        eh_obj_t libc;
        eh_find_obj(&libc, "*libc.so*");
        eh_find_sym(&libc, "dlsym", (void **) &dlsym_ptr);
        eh_destroy_obj(&libc);
    }

    if (strcmp(symbol, "eglGetProcAddress") == 0) {
        return (void*)eglGetProcAddress;
    } else if (strcmp(symbol, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } else if (strcmp(symbol, "eglTerminate") == 0) {
        return (void*)eglTerminate;
    } else if (strcmp(symbol, "glXGetProcAddress") == 0) {
        return (void*)glXGetProcAddress;
    } else if (strcmp(symbol, "glXGetProcAddressARB") == 0) {
        return (void*)glXGetProcAddressARB;
    } else if (strcmp(symbol, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp(symbol, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    } 

    return dlsym_ptr(handle, symbol);
}
