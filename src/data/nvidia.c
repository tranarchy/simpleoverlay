#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>

#include <nvml.h>
#include <common.h>

typedef nvmlReturn_t (*PFNNVMLINITV2)(void);
typedef nvmlReturn_t (*PFNNVMLDEVICEGETHANDLEBYINDEX)(unsigned int index, nvmlDevice_t *device);
typedef nvmlReturn_t (*PFNNVMLDEVICEGETTEMPERATURE)(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int *temp);
typedef nvmlReturn_t (*PFNNVMLDEVICEGETMEMORYINFO)(nvmlDevice_t device, nvmlMemory_t *memory);
typedef nvmlReturn_t (*PFNNVMLDEVICEGETUTILIZATIONRATES)(nvmlDevice_t device, nvmlUtilization_t *utilization);

static PFNNVMLDEVICEGETTEMPERATURE nvmlDeviceGetTemperature_ptr = NULL;
static PFNNVMLDEVICEGETMEMORYINFO nvmlDeviceGetMemoryInfo_ptr = NULL;
static PFNNVMLDEVICEGETUTILIZATIONRATES nvmlDeviceGetUtilizationRates_ptr = NULL;

static const char *nvml_libs[] = { "libnvidia-ml.so.1", "libnvidia-ml.so" };

static nvmlDevice_t device = NULL;
static nvmlReturn_t ret = -1;

void get_nvidia_usage(s_overlay_info *overlay_info) {
     nvmlUtilization_t usage;

     nvmlDeviceGetUtilizationRates_ptr(device, &usage);

     overlay_info->gpu_usage = usage.gpu;
}

void get_nvidia_mem_usage(s_overlay_info *overlay_info) {
    nvmlMemory_t mem;

    nvmlDeviceGetMemoryInfo_ptr(device, &mem);

    overlay_info->gpu_mem = mem.used / (1024.0 * 1024.0 * 1024.0);
}

void get_nvidia_temp(s_overlay_info *overlay_info) {
    unsigned int temp;
    nvmlDeviceGetTemperature_ptr(device, NVML_TEMPERATURE_GPU, &temp);

    overlay_info->gpu_temp = (int)temp;
}

void populate_nvidia(s_overlay_info *overlay_info) {
    if (ret != NVML_SUCCESS) {
      void *nvml_handle = NULL;
    
      for (size_t i = 0; i < sizeof(nvml_libs) / sizeof(nvml_libs[0]); i++) {
        nvml_handle = dlopen(nvml_libs[i], RTLD_LAZY);

        if (nvml_handle) {
            break;
        }
      }

      if (!nvml_handle) {
        return;
      }
      
      PFNNVMLINITV2 nvmlInit_v2_ptr = dlsym(nvml_handle, "nvmlInit_v2");
      PFNNVMLDEVICEGETHANDLEBYINDEX nvmlDeviceGetHandleByIndex_ptr = dlsym(nvml_handle, "nvmlDeviceGetHandleByIndex");
      nvmlDeviceGetTemperature_ptr = dlsym(nvml_handle, "nvmlDeviceGetTemperature");
      nvmlDeviceGetMemoryInfo_ptr = dlsym(nvml_handle, "nvmlDeviceGetMemoryInfo");
      nvmlDeviceGetUtilizationRates_ptr = dlsym(nvml_handle, "nvmlDeviceGetUtilizationRates");
      
      ret = nvmlInit_v2_ptr();

      nvmlDeviceGetHandleByIndex_ptr(0, &device);
    }

    get_nvidia_usage(overlay_info);
    get_nvidia_mem_usage(overlay_info);
    get_nvidia_temp(overlay_info);
}
