#include <dlfcn.h>
#include <string.h>

#include <glad.h>

void cleanup(void);
void draw_overlay(const char *interface, unsigned int *viewport);

typedef void *(*PFNDLSYM)(void *handle, const char *symbol);

typedef unsigned int (*PFNEGLSWAPBUFFERS)(void *display, void *surface);
typedef void *(*PFNEGLGETPROCADDRESS)(const char *procname);
typedef unsigned int (*PFNEGLTERMINATE)(void *display);
typedef void *(*PFNEGLGETCURRENTCONTEXT)(void);
typedef unsigned int (*PFNEGLMAKECURRENT)(void *display, void *draw, void *read, void *context);
typedef unsigned int (*PFNEGLQUERYSURFACE)(void *display, void *surface, int attribute, int *value);
typedef void *(*PFNEGLCREATECONTEXT)(void *display, void *config, void* share_context, const int *attrib_list);
typedef unsigned int (*PFNEGLDESTROYCONTEXT)(void *display, void *context);

static PFNEGLSWAPBUFFERS eglSwapBuffers_ptr = NULL;
static PFNEGLGETPROCADDRESS eglGetProcAddress_ptr = NULL;
static PFNEGLTERMINATE eglTerminate_ptr = NULL;
static PFNEGLGETCURRENTCONTEXT eglGetCurrentContext_ptr = NULL;
static PFNEGLMAKECURRENT eglMakeCurrent_ptr = NULL;
static PFNEGLQUERYSURFACE eglQuerySurface_ptr = NULL;
static PFNEGLDESTROYCONTEXT eglDestroyContext_ptr = NULL;

extern PFNDLSYM dlsym_ptr;

static void *egl_handle = NULL;
static void *egl_ctx, *prev_egl_ctx = NULL;

static const char *egl_libs[] = { "libEGL.so.1", "libEGL.so" };

static void *get_libegl_addr(const char *proc_name) {
  if (!egl_handle) {
    for (size_t i = 0; i < sizeof(egl_libs) / sizeof(egl_libs[0]); i++) {
       egl_handle = dlopen(egl_libs[i], RTLD_LAZY);

       if (egl_handle) {
          break;
       }
    } 
  }

  return dlsym_ptr(egl_handle, proc_name);
}

static void *get_egl_ctx(void *display) {
     if (egl_ctx) {
        return egl_ctx;
     }

    PFNEGLCREATECONTEXT eglCreateContext_ptr = (PFNEGLCREATECONTEXT)get_libegl_addr("eglCreateContext");

    egl_ctx = eglCreateContext_ptr(display, NULL, (void*)0, NULL);

    return egl_ctx;
}

unsigned int eglSwapBuffers(void *display, void *surf) {
    if (!eglSwapBuffers_ptr) {
      gladLoadGL();
      
      eglSwapBuffers_ptr = (PFNEGLSWAPBUFFERS)get_libegl_addr("eglSwapBuffers");
      eglMakeCurrent_ptr = (PFNEGLMAKECURRENT)get_libegl_addr("eglMakeCurrent");
      eglQuerySurface_ptr = (PFNEGLQUERYSURFACE)get_libegl_addr("eglQuerySurface");
      eglGetCurrentContext_ptr = (PFNEGLGETCURRENTCONTEXT)get_libegl_addr("eglGetCurrentContext");
    }


    prev_egl_ctx = eglGetCurrentContext_ptr();

    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    eglQuerySurface_ptr(display, surf, 0x3057, &viewport[2]);
    eglQuerySurface_ptr(display, surf, 0x3056, &viewport[3]);

    eglMakeCurrent_ptr(display, surf, surf, get_egl_ctx(display));
    
    draw_overlay("EGL", (unsigned int*)viewport);
    
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

void *eglGetProcAddress(char const *procname) {
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
