#include <stdbool.h>

typedef struct s_config {
  float scale;

  int bg_color[4];
  int key_color[4];
  int value_color[4];

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

