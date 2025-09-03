#include <stdbool.h>

typedef struct s_config {
  float scale;
  
  float key_color[3];
  float value_color[3];

  bool fps_only;
} s_config;

typedef struct s_overlay_info {
    int fps;
    float frametime;

    int cpu_usage;
    int cpu_temp;

    int gpu_usage;
    float gpu_mem;
    int gpu_temp;

    float mem;
} s_overlay_info;

