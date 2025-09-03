#include <stdio.h>
#include <string.h>

#include "../include/common.h"

#if !defined(__linux__)
  #include <unistd.h>
  
  #include <sys/sysctl.h>
  #include <sys/vmmeter.h>

  #if defined(__FreeBSD__)
      #include <vm/vm_param.h>
  #endif
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
#else
  static void get_mem_usage(s_overlay_info *overlay_info) {
      int mib[2];

      size_t len;
      size_t slen;
      long long total;
      struct vmtotal vmtotal;

      mib[0] = CTL_HW;

      #if defined(__FreeBSD__)
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
#endif

void populate_mem(s_overlay_info *overlay_info) {
    get_mem_usage(overlay_info);
}

