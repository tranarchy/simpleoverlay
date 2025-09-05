#include <stdlib.h>

#include "include/common.h"

#if defined(__APPLE__)
  void hook_cgl(void);
#endif

s_config config;

static void hex_to_rgb(const char *hex, float *rgb) {
  long hex_num = strtol(hex, NULL, 16);

  rgb[0] = ((hex_num >> 16) & 0xFF);
  rgb[1] = ((hex_num >> 8) & 0xFF);
  rgb[2] = (hex_num & 0xFF);
}

__attribute__((constructor))
static void init(void) {
  config.scale = getenv("SCALE") ? atof(getenv("SCALE")) : 1.8f;
  hex_to_rgb(getenv("KEY_COLOR") ? getenv("KEY_COLOR") : "9889FA", config.key_color);
  hex_to_rgb(getenv("VALUE_COLOR") ? getenv("VALUE_COLOR") : "FFFFFF", config.value_color);
  config.fps_only = getenv("FPS_ONLY") ? atoi(getenv("FPS_ONLY")) : false;

  #if defined(__APPLE__)
    hook_cgl();
  #endif
}

