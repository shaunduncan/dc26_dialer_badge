#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include <stdbool.h>

#include <ws2811.h>

// Effect func typdef
typedef void (*effect)(ws2811_t* np);
typedef bool (*active_func)();

// Utilities
int rgb2int(int r, int g, int b);
int hsv2rgb(int h, double s, double v);

// Effects
void effect_clear(ws2811_t*);

// Sweep effects
void effect_unicorn_dial(ws2811_t*, active_func);
void effect_comet_dial(ws2811_t*, active_func);
void effect_comet_color_cycle_dial(ws2811_t*, active_func);
void effect_comet_rainbow_trail_dial(ws2811_t*, active_func);
void effect_comet_rainbow_reveal_dial(ws2811_t*, active_func);
void effect_full_rainbow_reveal_dial(ws2811_t*, active_func);
void effect_full_color_dial(ws2811_t*, active_func);
void effect_full_rainbow_wipe_dial(ws2811_t*, active_func);
void effect_fire_ring(ws2811_t*, active_func);
void effect_random_fire_ring(ws2811_t*, active_func);
void random_sweep_effect(ws2811_t*, active_func);

// Strobe effects
void effect_strobe(ws2811_t*);
void effect_random_strobe(ws2811_t*);
void effect_rainbow_strobe(ws2811_t*);
void effect_rainbow_static_strobe(ws2811_t*);
void effect_rainbow_dynamic_strobe(ws2811_t*);

// Twinkle effects
void effect_twinkle(ws2811_t*);
void effect_rainbow_random_twinkle(ws2811_t*);
void effect_rainbow_fixed_twinkle(ws2811_t*);

// Digit hightlight effect
void effect_dial_digit_highlight(ws2811_t*, int);

#endif
