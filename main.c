#include <stdlib.h>

#include <common.h>

#if defined(__APPLE__)
  void hook_cgl(void);
#endif

s_config config;

static void hex_to_rgba(const char *hex, int *rgba) {
  unsigned long hex_num = strtoul(hex, NULL, 16);

  rgba[0] = ((hex_num >> 24) & 0xFF);
  rgba[1] = ((hex_num >> 16) & 0xFF);
  rgba[2] = ((hex_num >> 8) & 0xFF);
  rgba[3] = (hex_num & 0xFF);
}

__attribute__((constructor))
static void init(void) {
  config.scale = getenv("SCALE") ? atof(getenv("SCALE")) : 1.8f;

  config.pos_x = getenv("HORIZONTAL_POS") ? atoi(getenv("HORIZONTAL_POS")) : LEFT_X;
  config.pos_y = getenv("VERTICAL_POS") ? atoi(getenv("VERTICAL_POS")) : TOP_Y;

  hex_to_rgba(getenv("BG_COLOR") ? getenv("BG_COLOR") : "10101A96", config.bg_color);
  hex_to_rgba(getenv("KEY_COLOR") ? getenv("KEY_COLOR") : "9889FAFF", config.key_color);
  hex_to_rgba(getenv("VALUE_COLOR") ? getenv("VALUE_COLOR") : "FFFFFFFF", config.value_color);
  
  config.fps_only = getenv("FPS_ONLY") ? atoi(getenv("FPS_ONLY")) : false;
  
  #if defined(__APPLE__)
    hook_cgl();
  #endif
}

