#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>

#include "../include/overlay.h"

static amdgpu_device_handle device = NULL;

static void get_amdgpu_usage(amdgpu_device_handle device,s_overlay_info *overlay_info) {
    size_t len = sizeof(overlay_info->gpu_usage);
    
    amdgpu_query_sensor_info(device, AMDGPU_INFO_SENSOR_GPU_LOAD, len, &overlay_info->gpu_usage);
}

static void get_amdgpu_mem_usage(amdgpu_device_handle device, s_overlay_info *overlay_info) {
    long long mem;
    size_t len;

    len = sizeof(mem);
    int ret = amdgpu_query_info(device, AMDGPU_INFO_VRAM_USAGE, len, &mem);

    if (ret < 0) {
        return;
    }

    overlay_info->gpu_mem = mem / (1024.0 * 1024.0 * 1024.0);
}

static void get_amdgpu_temp(amdgpu_device_handle device, s_overlay_info *overlay_info) {
    size_t len = sizeof(overlay_info->gpu_temp);

    int ret = amdgpu_query_sensor_info(device, AMDGPU_INFO_SENSOR_GPU_TEMP, len, &overlay_info->gpu_temp);

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
        fd = open("/dev/dri/renderD128", O_RDONLY);
        if (!amdgpu_device_initialize(fd, &major_version, &minor_version, &device)) {
            return;
        }
        close(fd);
    }

    get_amdgpu_usage(device, overlay_info);
    get_amdgpu_mem_usage(device, overlay_info);
    get_amdgpu_temp(device, overlay_info);
}

