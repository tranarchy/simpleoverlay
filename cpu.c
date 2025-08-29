#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "include/overlay.h"

#if defined(__linux__)
    #define CPU_IDLE_STATE 3
    #define CPU_STATES_NUM 10
#elif defined(__OpenBSD__)
    #include <sys/time.h>
    #include <sys/sysctl.h>
    #include <sys/sensors.h>

    #define CPU_IDLE_STATE 5 
    #define CPU_STATES_NUM 6
#elif defined(__FreeBSD__) || defined(__NetBSD__)
    #include <sys/sysctl.h>

    #define CPU_IDLE_STATE 4
    #define CPU_STATES_NUM 5
#endif

static long long cpu_usage[CPU_STATES_NUM];
static long long cpu_usage_prev[CPU_STATES_NUM] = { 0 };

static void get_cpu_usage_percent(s_overlay_info *overlay_info, long long *cpu_usage, long long *cpu_usage_prev, int cpu_states_num, int cpu_idle_state) {
    long long cpu_usage_sum = 0;
    long long cpu_non_idle_usage_sum = 0;
    
    for (int i = 0; i < cpu_states_num; i++) {
        cpu_usage_sum += cpu_usage[i] - cpu_usage_prev[i];
    }

    long long cpu_idle = cpu_usage[cpu_idle_state] - cpu_usage_prev[cpu_idle_state];
    cpu_non_idle_usage_sum = cpu_usage_sum - cpu_idle;

    overlay_info->cpu_usage = 100.0f * cpu_non_idle_usage_sum / cpu_usage_sum;
        
    for (int i = 0; i < cpu_states_num; i++) {
        cpu_usage_prev[i] = cpu_usage[i];
    }
}

#if defined(__linux__)
    static void get_cpu_usage(s_overlay_info *overlay_info) {
        FILE *fp;
        char line[512];

        fp = fopen("/proc/stat", "r");

        if (fp == NULL) {
            fclose(fp);
            return;
        }

        fgets(line, sizeof(line), fp);
        fclose(fp);

        sscanf(
            line,
            "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
            &cpu_usage[0], &cpu_usage[1], &cpu_usage[2], &cpu_usage[3], &cpu_usage[4], &cpu_usage[5], &cpu_usage[6], &cpu_usage[7], &cpu_usage[8], &cpu_usage[9]
        );

        get_cpu_usage_percent(overlay_info, cpu_usage, cpu_usage_prev, CPU_STATES_NUM, CPU_IDLE_STATE);
    }

    static void get_cpu_temp(s_overlay_info *overlay_info) {
        FILE *fp;

        char temp_buff[16];

        fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");

        if (fp == NULL) {
            return;
        }

        fgets(temp_buff, 16, fp);
        fclose(fp);

        overlay_info->cpu_temp = atoi(temp_buff) / 1000;
    }
    
#elif defined(__OpenBSD__) || defined(__NetBSD__)

    static void get_cpu_usage(s_overlay_info *overlay_info) {
        int mib[2];

        mib[0] = CTL_KERN;
        #if defined(__OpenBSD__)
            mib[1] = KERN_CPTIME;
        #else
            mib[1] = KERN_CP_TIME;
        #endif

        size_t len = sizeof(cpu_usage);
        sysctl(mib, 2, &cpu_usage, &len, NULL, 0);

        get_cpu_usage_percent(overlay_info, cpu_usage, cpu_usage_prev, CPU_STATES_NUM, CPU_IDLE_STATE);
    }

    #if defined(__OpenBSD__)
        static struct sensor get_sensor(char *sensor_name, int sensor_type) {
            int ret;
            size_t len, slen;
            struct sensor sensor;
            struct sensordev sensordev;

            int mib[5];

            mib[0] = CTL_HW;
            mib[1] = HW_SENSORS;

            for (int i = 0; i < 1024; i++) {

                mib[2] = i;

                len = sizeof(struct sensordev);
                ret = sysctl(mib, 3, &sensordev, &len, NULL, 0);

                if (ret == -1) {
                    continue;
                }

                mib[3] = sensor_type;

                for (int j = 0; j < sensordev.maxnumt[sensor_type]; j++) {
                    mib[4] = j;
                    slen = sizeof(struct sensor);
                    ret = sysctl(mib, 5, &sensor, &slen, NULL, 0);

                    if (ret == -1) {
                        continue;
                    }

                    if (strcmp(sensordev.xname, sensor_name) == 0) {
                        return sensor;
                    }

                }
            }

            sensor.value = -1;

            return sensor;
        }

        static void get_cpu_temp(s_overlay_info *overlay_info) {
    
            struct sensor temp_sensor = get_sensor("cpu0", SENSOR_TEMP);

            if (temp_sensor.value == -1) {
                return;
            }

            overlay_info->cpu_temp = (temp_sensor.value - 273150000) / 1E6;;
        }
    #else
        static void get_cpu_temp(s_overlay_info *overlay_info) {
            // todo implement cpu temp for netbsd
        }
    
    #endif
#elif defined(__FreeBSD__)
    static void get_cpu_usage(s_overlay_info *overlay_info) {
        size_t len;

        sysctlbyname("kern.cp_time", &cpu_usage, &len, NULL, 0);
        get_cpu_usage_percent(overlay_info, cpu_usage, cpu_usage_prev, CPU_STATES_NUM, CPU_IDLE_STATE);
    }

    static void get_cpu_temp(s_overlay_info *overlay_info) {
    
        size_t len;

        len = sizeof(overlay_info->cpu_temp);
        int ret = sysctlbyname("dev.cpu.0.temperature", &overlay_info->cpu_temp, &len, NULL, 0);

        if (ret == -1) {
            return;
        }

        overlay_info->cpu_temp = overlay_info->cpu_temp / 10 - 273.15;
    }
#endif


void populate_cpu(s_overlay_info *overlay_info) {
    get_cpu_usage(overlay_info);
    get_cpu_temp(overlay_info);
}

