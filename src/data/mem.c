#include <stdio.h>
#include <string.h>

#include <common.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
  #include <unistd.h>
  #include <sys/sysctl.h>

  #if defined(__OpenBSD__)
    #include <uvm/uvmexp.h>
  #elif defined(__NetBSD__)
    #include <uvm/uvm_extern.h>
  #endif
  
#elif defined(__APPLE__)
    #include <unistd.h>
    #include <mach/mach.h>
#elif defined(__illumos__)
    #include <unistd.h>
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

      overlay_info->mem = (total - available) / (1024.0 * 1024.0);
  }
#elif defined(__FreeBSD__) || defined(__DragonFly__)
  static void get_mem_usage(s_overlay_info *overlay_info) {
      int mib[4];
      
      size_t len, slen;

      long long total;
      int free, inactive, cache;

      long long pagesize = sysconf(_SC_PAGESIZE);

      mib[0] = CTL_HW;
      mib[1] = HW_PHYSMEM;

      len = sizeof(total);
      sysctl(mib, 2, &total, &len, NULL, 0);

      len = 4;
      slen = sizeof(free);
      sysctlnametomib("vm.stats.vm.v_free_count", mib, &len);
      sysctl(mib, 4, &free, &slen, NULL, 0);

      len = 4;
      slen = sizeof(inactive);
      sysctlnametomib("vm.stats.vm.v_inactive_count", mib, &len);
      sysctl(mib, 4, &inactive, &slen, NULL, 0);

      len = 4;
      slen = sizeof(cache);
      sysctlnametomib("vm.stats.vm.v_cache_count", mib, &len);
      sysctl(mib, 4, &cache, &slen, NULL, 0);

      long long used = (free + inactive + cache) * pagesize;
      overlay_info->mem = (total - used) / (1024.0 * 1024.0 * 1024.0);
  }
#elif defined(__OpenBSD__) || defined(__NetBSD__)
  static void get_mem_usage(s_overlay_info *overlay_info) {
      int mib[2];

      mib[0] = CTL_VM;

      #if defined(__OpenBSD__)
        struct uvmexp uvmexp;
        mib[1] = VM_UVMEXP;
      #else
        struct uvmexp_sysctl uvmexp;
        mib[1] = VM_UVMEXP2;
      #endif
      
      long long pagesize = sysconf(_SC_PAGESIZE);

      size_t len = sizeof(uvmexp);

      sysctl(mib, 2, &uvmexp, &len, NULL, 0);

      long long used = (uvmexp.active + uvmexp.inactive + uvmexp.wired) * pagesize;
      overlay_info->mem = used / (1024.0 * 1024.0 * 1024.0);
  }
#elif defined(__APPLE__)
  static void get_mem_usage(s_overlay_info *overlay_info) {
      vm_statistics64_data_t vm_stats; 
      mach_msg_type_number_t slen = HOST_VM_INFO64_COUNT;

      int ret = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &slen);

      if (ret == -1) {
        return;
      }

      long long pagesize = sysconf(_SC_PAGESIZE);

      long long used = (vm_stats.active_count + vm_stats.inactive_count +
              vm_stats.speculative_count + vm_stats.wire_count +
              vm_stats.compressor_page_count - vm_stats.purgeable_count - vm_stats.external_page_count) * pagesize;

      overlay_info->mem = used / (1024.0 * 1024.0 * 1024.0);
  }
#elif defined(__illumos__)
  static void get_mem_usage(s_overlay_info *overlay_info) {
      long long pagesize = sysconf(_SC_PAGESIZE);
    
      long long total = sysconf(_SC_PHYS_PAGES) * pagesize;
      long long available = sysconf(_SC_AVPHYS_PAGES) * pagesize;

      overlay_info->mem = (total - available) / (1024.0 * 1024.0 * 1024.0);
  }
#endif

void populate_mem(s_overlay_info *overlay_info) {
    get_mem_usage(overlay_info);
}

