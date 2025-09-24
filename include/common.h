#include <stdbool.h>

typedef enum VERTICAL {
    TOP_Y,
    BOTTOM_Y,
    CENTER_Y
} VERTICAL;

typedef enum HORIZONTAL {
    LEFT_X,
    RIGHT_X,
    CENTER_X
} HORIZONTAL;

typedef struct s_config {
  float scale;

  int bg_color[4];
  int key_color[4];
  int value_color[4];
  bool rainbow;

  VERTICAL pos_y;
  HORIZONTAL pos_x;

  char metrics;

  bool no_graph;

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

