#include <stdbool.h>

#include <objc/runtime.h>
#include <objc/message.h>

#include "../include/glad.h"

void draw_overlay(const char *gl_interface, unsigned int *viewport);

static IMP flushBuffer_ptr;

static bool glad_init = false;

static void flushBuffer(id self, SEL _cmd) {
  if (!glad_init) {
      gladLoadGL();
      glad_init = true;
  }

  int viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  draw_overlay("CGL", (unsigned int*)viewport);

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