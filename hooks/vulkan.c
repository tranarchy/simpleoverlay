#include <dlfcn.h>
#include <string.h>

void draw_overlay(const char *interface, unsigned int *viewport);

typedef void *(*PFNDLSYM)(void *handle, const char *symbol);

typedef void *(*PFNVKQUEUEPRESENTKHR)(void *queue, void *pPresentInfo);
typedef void *(*PFNVKGETINSTANCEPROCADDR)(void *instance, const char* pName);
typedef void *(*PFNVKGETDEVICEPROCADDR)(void *device, const char* pName);

PFNVKQUEUEPRESENTKHR vkQueuePresentKHR_ptr = NULL;
PFNVKGETINSTANCEPROCADDR vkGetInstanceProcAddr_ptr = NULL;
PFNVKGETDEVICEPROCADDR vkGetDeviceProcAddr_ptr = NULL;

extern PFNDLSYM dlsym_ptr;

static const char *vk_libs[] = { "libvulkan.so.1", "libvulkan.so" };

static void *vk_handle = NULL;

static void *get_libvk_addr(const char *proc_name) {
  if (!vk_handle) {
    for (size_t i = 0; i < sizeof(vk_libs) / sizeof(vk_libs[0]); i++) {
       vk_handle = dlopen(vk_libs[i], RTLD_LAZY);

       if (vk_handle) {
          break;
       }
    } 
  }

  return dlsym_ptr(vk_handle, proc_name);
}

void *vkQueuePresentKHR(void *queue, void* pPresentInfo) {
    if (!vkQueuePresentKHR_ptr) {
      vkQueuePresentKHR_ptr = (PFNVKQUEUEPRESENTKHR)get_libvk_addr("vkQueuePresentKHR");
    }

    // to do: implement VK backend for micrui

    return vkQueuePresentKHR_ptr(queue, pPresentInfo);
}

void *vkGetInstanceProcAddr(void *instance, const char* pName) {
    if (strcmp(pName, "vkQueuePresentKHR") == 0) {
      return (void*)vkQueuePresentKHR;
    }

    if (!vkGetInstanceProcAddr_ptr) {
      vkGetInstanceProcAddr_ptr = (PFNVKGETINSTANCEPROCADDR)get_libvk_addr("vkGetInstanceProcAddr");
    }

    return vkGetInstanceProcAddr_ptr(instance, pName);
}

void *vkGetDeviceProcAddr(void *device, const char* pName) {
    if (strcmp(pName, "vkGetInstanceProcAddr") == 0) {
      return (void*)vkGetInstanceProcAddr;
    } else if (strcmp(pName, "vkQueuePresentKHR") == 0) {
      return (void*)vkQueuePresentKHR;
    }
    
    if (!vkGetDeviceProcAddr_ptr) {
      vkGetDeviceProcAddr_ptr = (PFNVKGETDEVICEPROCADDR)get_libvk_addr("vkGetDeviceProcAddr");
    }

    return vkGetDeviceProcAddr_ptr(device, pName);
}
