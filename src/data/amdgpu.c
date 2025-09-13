#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <common.h>

#define AMDGPU_INFO_SENSOR_GPU_LOAD 0x4
#define AMDGPU_INFO_VRAM_USAGE 0x10
#define AMDGPU_INFO_SENSOR_GPU_TEMP 0x3

typedef int (*PFNAMDGPUDEVICEINITIALIZE)(int fd, uint32_t *major_version, uint32_t *minor_version, void **device_handle);
typedef int (*PFNAMDGPUQUERYSENSORINFO)(void *dev, unsigned int sensor_type, size_t size, void *value);
typedef int (*PFNAMDGPUQUERYINFO)(void *dev, unsigned int info_id, size_t size, void *value);

static PFNAMDGPUQUERYSENSORINFO amdgpu_query_sensor_info_ptr = NULL;
static PFNAMDGPUQUERYINFO amdgpu_query_info_ptr = NULL;

static void *device = NULL;

static void get_amdgpu_usage(void **device, s_overlay_info *overlay_info) {
    size_t len = sizeof(overlay_info->gpu_usage);
    
    amdgpu_query_sensor_info_ptr(device, AMDGPU_INFO_SENSOR_GPU_LOAD, len, &overlay_info->gpu_usage);
}

static void get_amdgpu_mem_usage(void **device, s_overlay_info *overlay_info) {
    long long mem;
    size_t len;

    len = sizeof(mem);
    int ret = amdgpu_query_info_ptr(device, AMDGPU_INFO_VRAM_USAGE, len, &mem);

    if (ret < 0) {
        return;
    }

    overlay_info->gpu_mem = mem / (1024.0 * 1024.0 * 1024.0);
}

static void get_amdgpu_temp(void **device, s_overlay_info *overlay_info) {
    size_t len = sizeof(overlay_info->gpu_temp);

    int ret = amdgpu_query_sensor_info_ptr(device, AMDGPU_INFO_SENSOR_GPU_TEMP, len, &overlay_info->gpu_temp);

    if (ret < 0) {
        return;
    }

    overlay_info->gpu_temp /= 1000;
}

void populate_amdgpu(s_overlay_info *overlay_info) {
    int fd;
    uint32_t major_version;
    uint32_t minor_version;

    if (!device) {
        PFNAMDGPUDEVICEINITIALIZE amdgpu_device_initialize_ptr;
        void *handle = dlopen("libdrm_amdgpu.so", RTLD_LAZY);
        amdgpu_device_initialize_ptr = (PFNAMDGPUDEVICEINITIALIZE)dlsym(handle, "amdgpu_device_initialize");
        amdgpu_query_info_ptr = (PFNAMDGPUQUERYINFO)dlsym(handle, "amdgpu_query_info");
        amdgpu_query_sensor_info_ptr = (PFNAMDGPUQUERYSENSORINFO)dlsym(handle, "amdgpu_query_sensor_info");
        
        fd = open("/dev/dri/renderD128", O_RDONLY);
        if (amdgpu_device_initialize_ptr(fd, &major_version, &minor_version, &device) < 0) {
            close(fd);
            return;
        }
        close(fd);
    }

    get_amdgpu_usage(device, overlay_info);
    get_amdgpu_mem_usage(device, overlay_info);
    get_amdgpu_temp(device, overlay_info);
}

