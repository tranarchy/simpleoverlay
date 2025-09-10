#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>

#include "../include/elfhacks.h"

unsigned int eglSwapBuffers(void *display, void *surf);
unsigned int eglTerminate(void *display);
void *eglGetProcAddress(char const *procname);

void glXSwapBuffers(void *dpy, void *drawable);
void glXDestroyContext(void *dpy, void *ctx);
void *glXGetProcAddress(const unsigned char *procName);
void *glXGetProcAddressARB(const unsigned char *procName);

void *vkQueuePresentKHR(void *queue, void* pPresentInfo);
void *vkGetInstanceProcAddr(void *instance, const char* pName);
void *vkGetDeviceProcAddr(void *device, const char* pName);

typedef void *(*PFNDLSYM)(void *handle, const char *symbol);

PFNDLSYM dlsym_ptr = NULL;

static bool angle = false;

void *dlsym(void *handle, const char *symbol) {
    if (!dlsym_ptr) {
        #if defined(__FreeBSD__) || defined(__DragonFly__)
            dlsym_ptr = (PFNDLSYM)&dlfunc;
        #else
            eh_obj_t libc;
            eh_find_obj(&libc, "*libc.so*");
            eh_find_sym(&libc, "dlsym", (void **) &dlsym_ptr);
            eh_destroy_obj(&libc);
        #endif
    }

    if (strstr(symbol, "ANGLE")) {
        angle = true;
    }

    if (strcmp(symbol, "glXGetProcAddress") == 0) {
        return (void*)glXGetProcAddress;
    } else if (strcmp(symbol, "glXGetProcAddressARB") == 0) {
        return (void*)glXGetProcAddressARB;
    } else if (strcmp(symbol, "glXSwapBuffers") == 0) {
        return (void*)glXSwapBuffers;
    } else if (strcmp(symbol, "glXDestroyContext") == 0) {
        return (void*)glXDestroyContext;
    }

    if (angle) {
        return dlsym_ptr(handle, symbol);
    }

    if (strcmp(symbol, "eglGetProcAddress") == 0) {
        return (void*)eglGetProcAddress;
    } else if (strcmp(symbol, "eglSwapBuffers") == 0) {
        return (void*)eglSwapBuffers;
    } else if (strcmp(symbol, "eglTerminate") == 0) {
        return (void*)eglTerminate;
    } else if (strcmp(symbol, "vkGetInstanceProcAddr") == 0) {
        return (void*)vkGetInstanceProcAddr;
    } else if (strcmp(symbol, "vkGetDeviceProcAddr") == 0) {
        return (void*)vkGetDeviceProcAddr;
    } else if (strcmp(symbol, "vkQueuePresentKHR") == 0) {
        return (void*)vkQueuePresentKHR;
    }

    return dlsym_ptr(handle, symbol);
}
