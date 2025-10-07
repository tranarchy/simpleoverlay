#include <microui.h>

void gl1_init(void);
void gl1_flush(mu_Rect rect, unsigned int* viewport);
void gl1_draw_rect(mu_Rect rect, mu_Color color);
void gl1_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

int gl1_get_text_height(void);
int gl1_get_text_width(const char *text, int len);

void gl3_init(void);
void gl3_flush(mu_Rect rect, unsigned int* viewport);
void gl3_draw_rect(mu_Rect rect, mu_Color color);
void gl3_draw_text(const char *text, mu_Vec2 pos, mu_Color color);

int gl3_get_text_height(void);
int gl3_get_text_width(const char *text, int len);

