#include <string.h>
#include <dlfcn.h>

#include "../include/elfhacks.h"

unsigned int eglSwapBuffers(void *display, void *surf);
unsigned int eglTerminate(void *display);
void *eglGetProcAddress(char const *procname);

void glXSwapBuffers(void *dpy, void *drawable);
void glXDestroyContext(void *dpy, void *ctx);
void *glXGetProcAddress(const unsigned char *procName);
void *glXGetProcAddressARB(const unsigned char *procName);

typedef void *(*PFNDLSYM)(void *handle, const char *symbol);

PFNDLSYM dlsym_ptr = NULL;

void *dlsym(void *handle, const char *symbol) {
    if (!dlsym_ptr) {
        #if defined(__FreeBSD__)
            dlsym_ptr = (PFNDLSYM)&dlfunc;
        #else
            eh_obj_t libc;
            eh_find_obj(&libc, "*libc.so*");
            eh_find_sym(&libc, "dlsym", (void **) &dlsym_ptr);
            eh_destroy_obj(&libc);
        #endif
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
