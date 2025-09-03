#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>

#include "../include/glad.h"

void cleanup(void);
void draw_overlay(const char *gl_interface, unsigned int *viewport);

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

static PFNGLXSWAPBUFFERS glXSwapBuffers_ptr = NULL;
static PFNGLXGETPROCADDRESS glXGetProcAddress_ptr = NULL;
static PFNGLXGETPROCADDRESSARB glXGetProcAddressARB_ptr = NULL;
static PFNGLXDESTROYCONTEXT glXDestroyContext_ptr = NULL;
static PFNGLXGETCURRENTCONTEXT glXGetCurrentContext_ptr = NULL;
static PFNGLXQUERYDRAWABLE glXQueryDrawable_ptr = NULL;
static PFNGLXMAKECURRENT glXMakeCurrent_ptr = NULL;

extern PFNDLSYM dlsym_ptr;

static const char *gl_libs[] = { "libGL.so.1", "libGL.so" };

static void *gl_handle = NULL;
static void *glx_ctx, *prev_glx_ctx = NULL;

static void *get_libgl_addr(const char *proc_name) {
  if (!gl_handle) {
    for (size_t i = 0; i < sizeof(gl_libs) / sizeof(gl_libs[0]); i++) {
       gl_handle = dlopen(gl_libs[i], RTLD_LAZY);

       if (gl_handle) {
          break;
       }
    } 
  }

  return dlsym_ptr(gl_handle, proc_name);
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
    
    draw_overlay("GLX", viewport);
  
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

