#include <stdio.h>
#include <string.h>

#include "../include/common.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  #include <unistd.h>
  
  #include <sys/sysctl.h>
  #include <sys/vmmeter.h>

  #if defined(__FreeBSD__) || defined(__DragonFly__)
      #include <vm/vm_param.h>
  #endif
#elif defined(__APPLE__)
    #include <unistd.h>
    #include <mach/mach.h>
#endif

#if defined(__linux__)
  static void get_mem_usage(s_overlay_info *overlay_info) {

      int total = 0;
      int available = 0;
 
      FILE *fp;
      char line[128];

      fp = fopen("/proc/meminfo", "r");

      if (fp == NULL) {
          return;
      }

      while (fgets(line, sizeof(line), fp)) {
          if (strstr(line, "MemTotal:") != NULL) {
              sscanf(line, "MemTotal: %d kB", &total);
          } else if (strstr(line, "MemAvailable:") != NULL) {
              sscanf(line, "MemAvailable: %d kB", &available);
          }

          if (total && available) {
            break;
          }
      }

      fclose(fp);

      overlay_info->mem = total - available;
      overlay_info->mem /= (1024.0 * 1024.0);
  }
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  static void get_mem_usage(s_overlay_info *overlay_info) {
      int mib[2];

      size_t len;
      size_t slen;
      long long total;
      struct vmtotal vmtotal;

      mib[0] = CTL_HW;

      #if defined(__FreeBSD__) || defined(__DragonFly__)
          mib[1] = HW_PHYSMEM;
      #else
          mib[1] = HW_PHYSMEM64;
      #endif

      len = sizeof(total);
      int ret = sysctl(mib, 2, &total, &len, NULL, 0);

      if (ret == -1) {
          return;
      }

      mib[0] = CTL_VM;
      mib[1] = VM_METER;

      slen = sizeof(vmtotal);
      ret = sysctl(mib, 2, &vmtotal, &slen, NULL, 0);

      if (ret == -1) {
          return;
      }

      long long pagesize = sysconf(_SC_PAGESIZE);

      #if defined(__OpenBSD__)
          overlay_info->mem = (vmtotal.t_avm * pagesize) / (1024.0 * 1024.0 * 1024.0);
      #else
          long long free = vmtotal.t_free * pagesize;
          overlay_info->mem = (total - free) / (1024.0 * 1024.0 * 1024.0);       
      #endif
  }
#elif defined(__APPLE__)
    void get_mem_usage(s_overlay_info *overlay_info) {
        vm_statistics64_data_t vm_stats; 
        mach_msg_type_number_t slen = HOST_VM_INFO64_COUNT;

        int ret = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &slen);

        if (ret == -1) {
            return;
        }

        long long pagesize = sysconf(_SC_PAGESIZE);

        long long active = vm_stats.active_count * pagesize;
        long long wire = vm_stats.wire_count * pagesize;
        long long compression = vm_stats.compressor_page_count * pagesize;

        overlay_info->mem = (active + wire + compression) / (1024.0 * 1024.0 * 1024.0);
    }

#endif

void populate_mem(s_overlay_info *overlay_info) {
    get_mem_usage(overlay_info);
}

