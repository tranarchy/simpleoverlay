#include <dlfcn.h>

#include <objc/runtime.h>
#include <CoreFoundation/CoreFoundation.h>

#include <glad.h>

void draw_overlay(const char *interface, unsigned int *viewport);

typedef int (*PFNCGLSETSURFACE)(void *gl, int cid, int wid, int sid);
typedef int (*PFNCGLGETSURFACE)(void *gl, int *cid, int *wid, int *sid);
typedef void* (*PFNCGLGETPIXELFORMAT)(void *gl);
typedef int (*PFNCGLCREATECONTEXT)(void *pixel_format, void *share_context, void **gl);
typedef void *(*PFNCGLGETCURRENTCONTEXT)(void);
typedef int (*PFNCGLSETCURRENTCONTEXT)(void *gl);
typedef int (*PFNCGLGETSURFACEBOUNDS)(int cid, int wid, int sid, CGRect *bounds);

static PFNCGLSETSURFACE CGLSetSurface_ptr = NULL;
static PFNCGLGETSURFACE CGLGetSurface_ptr = NULL;
static PFNCGLGETPIXELFORMAT CGLGetPixelFormat_ptr = NULL;
static PFNCGLCREATECONTEXT CGLCreateContext_ptr = NULL;
static PFNCGLGETCURRENTCONTEXT CGLGetCurrentContext_ptr = NULL;
static PFNCGLSETCURRENTCONTEXT CGLSetCurrentContext_ptr = NULL;
static PFNCGLGETSURFACEBOUNDS CGLGetSurfaceBounds_ptr = NULL;

static IMP flushBuffer_ptr;

static int cid, wid, sid;

static void *cgl_handle = NULL;
static void *cgl_ctx, *prev_cgl_ctx = NULL;

static const char *cgl_libs[] = { 
  "/System/Library/Frameworks/OpenGL.framework/OpenGL", 
  "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL" 
};

static void *get_libcgl_addr(const char *proc_name) {
  if (!cgl_handle) {
    for (size_t i = 0; i < sizeof(cgl_libs) / sizeof(cgl_libs[0]); i++) {
       cgl_handle = dlopen(cgl_libs[i], RTLD_LAZY);

       if (cgl_handle) {
          break;
       }
    } 
  }

  return dlsym(cgl_handle, proc_name);
}

static void *get_cgl_ctx() {
  if (cgl_ctx) {
    return cgl_ctx;
  }
  
  CGLGetPixelFormat_ptr = (PFNCGLGETPIXELFORMAT)get_libcgl_addr("CGLGetPixelFormat");
  CGLCreateContext_ptr = (PFNCGLCREATECONTEXT)get_libcgl_addr("CGLCreateContext");

  void *pixel_format = CGLGetPixelFormat_ptr(prev_cgl_ctx);

  CGLCreateContext_ptr(pixel_format, 0, &cgl_ctx);

  return cgl_ctx;
}

static void flushBuffer(id self, SEL _cmd) {
  if (!CGLGetCurrentContext_ptr) {
      gladLoadGL();
      CGLGetCurrentContext_ptr = (PFNCGLGETCURRENTCONTEXT)get_libcgl_addr("CGLGetCurrentContext");
      CGLSetCurrentContext_ptr = (PFNCGLSETCURRENTCONTEXT)get_libcgl_addr("CGLSetCurrentContext");
      CGLGetSurface_ptr = (PFNCGLGETSURFACE)get_libcgl_addr("CGLGetSurface");
      CGLSetSurface_ptr = (PFNCGLSETSURFACE)get_libcgl_addr("CGLSetSurface");
      CGLGetSurfaceBounds_ptr = (PFNCGLGETSURFACEBOUNDS)get_libcgl_addr("CGSGetSurfaceBounds");
  }

  int viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  CGRect bounds;

  glFlush();

  prev_cgl_ctx = CGLGetCurrentContext_ptr();
  CGLGetSurface_ptr(prev_cgl_ctx, &cid, &wid, &sid);
  CGLGetSurfaceBounds_ptr(cid, wid, sid, &bounds);

  viewport[2] = (int)bounds.size.width;
  viewport[3] = (int)bounds.size.height;

  CGLSetSurface_ptr(get_cgl_ctx(), cid, wid, sid);
  CGLSetCurrentContext_ptr(get_cgl_ctx());
  draw_overlay("CGL", (unsigned int*)viewport);
  glFlush();

  CGLSetCurrentContext_ptr(prev_cgl_ctx);

  ((void(*)(id, SEL))flushBuffer_ptr)(self, _cmd);
}

void hook_cgl(void) {
  Class ns_opengl_ctx_class = objc_getClass("NSOpenGLContext");

  if (!ns_opengl_ctx_class) {
    return;
  }

  SEL flush_buffer_selector = sel_getUid("flushBuffer");

  Method original_flush_buffer_method = class_getInstanceMethod(ns_opengl_ctx_class, flush_buffer_selector);
  flushBuffer_ptr = method_getImplementation(original_flush_buffer_method);

  method_setImplementation(original_flush_buffer_method, (IMP)flushBuffer);
}
