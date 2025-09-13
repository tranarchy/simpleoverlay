#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

#include <common.h>

static void get_perf_stat(CFMutableDictionaryRef *perf_stat) {
    CFMutableDictionaryRef io_dict = IOServiceMatching("IOAccelerator");

    io_iterator_t iterator;

    if (IOServiceGetMatchingServices(kIOMainPortDefault, io_dict, &iterator) == kIOReturnSuccess)
    {
        io_registry_entry_t reg_entry;

        while ((reg_entry = IOIteratorNext(iterator))) {
            CFMutableDictionaryRef service_dict;
            if (IORegistryEntryCreateCFProperties(reg_entry, &service_dict, kCFAllocatorDefault, kNilOptions) != kIOReturnSuccess) {
                IOObjectRelease(reg_entry);
                continue;
            }

            *perf_stat = (CFMutableDictionaryRef) CFDictionaryGetValue(service_dict, CFSTR("PerformanceStatistics"));
        }
    }
}

static void get_applegpu_usage(CFMutableDictionaryRef perf_stat, s_overlay_info *overlay_info) {
    const void* usage_dict = CFDictionaryGetValue(perf_stat, CFSTR("Device Utilization %"));

    if (usage_dict) {
        CFNumberGetValue( (CFNumberRef) usage_dict, kCFNumberSInt64Type, &overlay_info->gpu_usage);
    }
}

static void get_applegpu_mem_usage(CFMutableDictionaryRef perf_stat, s_overlay_info *overlay_info) {
    const void* mem_dict = CFDictionaryGetValue(perf_stat, CFSTR("In use system memory"));

    ssize_t mem;

    if (mem_dict) {
        CFNumberGetValue( (CFNumberRef) mem_dict, kCFNumberSInt64Type, &mem);
        overlay_info->gpu_mem = mem / (1024.0 * 1024.0 * 1024.0);
    }
}

void populate_applegpu(s_overlay_info *overlay_info) {
    CFMutableDictionaryRef perf_stat = NULL;

    get_perf_stat(&perf_stat);

    if (!perf_stat) {
        return;
    }

    get_applegpu_usage(perf_stat, overlay_info);

    // there is no dedicated VRAM on macOS, the GPU just uses system memory, not sure if this is useful for anything
    get_applegpu_mem_usage(perf_stat, overlay_info);
}
