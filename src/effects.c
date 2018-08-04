#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ws2811.h>

#include "effects.h"


#define MIN(a,b) (((a) > (b)) ? (b) : (a))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define TICK 7500
#define TICK_CLEANUP ((TICK) / 5)
#define DIAL_MAX_SWEEP 2
#define STROBE_PHASE 15
#define STROBE_TICK (1000000 / STROBE_PHASE)
#define STROBE_MAX (STROBE_PHASE)
#define COMET_TRAIL_FACTOR 0.5
#define TWINKLE_SPARSE_FACTOR 1
#define TWINKLE_TICK ((TICK) * 5)

int rgb2int(int r, int g, int b)
{
    return (r << 16) | (g << 8) | b;
}

#define WHITE rgb2int(255,255,255)

int hsv2rgb(int h, double s, double v)
{
    h = fmod(h, 360);

    double c = v * s;
    double hp = h / 60.0;
    double x = c * (1 - fabs(fmod(hp, 2) - 1));

    double r = 0;
    double g = 0;
    double b = 0;

    if (hp <= 1)
    {
        r = c;
        g = x;
    }
    else if (hp <= 2)
    {
        r = x;
        g = c;
    }
    else if (hp <= 3)
    {
        g = c;
        b = x;
    }
    else if (hp <= 4)
    {
        g = x;
        b = c;
    }
    else if (hp <= 5)
    {
        r = x;
        b = c;
    }
    else if (hp <= 6)
    {
        r = c;
        b = x;
    }

    double m = v - c;

    int r0 = (int) (255 * (r + m));
    int g0 = (int) (255 * (g + m));
    int b0 = (int) (255 * (b + m));

    return rgb2int(r0, g0, b0);
}

int num_pixels(ws2811_t* np)
{
    return np->channel[0].count;
}

void set_pixel(ws2811_t* np, int index, uint32_t value)
{
	np->channel[0].leds[index] = value;
}

void set_all_pixels(ws2811_t* np, uint32_t color)
{
    for (int i = 0; i < num_pixels(np); i++)
        set_pixel(np, i, color);
}

int get_marker_width(ws2811_t* np)
{
    return (int) ((float) num_pixels(np) * COMET_TRAIL_FACTOR);
}

void effect_clear(ws2811_t* np)
{
    for (int i = 0; i < num_pixels(np); i++)
    {
        set_pixel(np, i, 0);
        ws2811_render(np);
        usleep(TICK_CLEANUP);
    }
}

void effect_comet_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);
    int marker_width = get_marker_width(np);

    int sweep = 0;
    int pos = 0;

    int color = rand();
    double fade_step = 1.0 / marker_width;

    while (active())
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        double v = 1.0;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, (pos - i) % pixels, i == 0 ? WHITE : hsv2rgb(color, 1.0, v));
            v -= fade_step;
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + marker_width; pos++)
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        double v = 1.0;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                set_pixel(np, (pos - i) % pixels, i == 0 ? WHITE : hsv2rgb(color, 1.0, v));
                v -= fade_step;
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_comet_color_cycle_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);
    int marker_width = get_marker_width(np);

    int sweep = 0;
    int pos = 0;

    int color = rand();
    double fade_step = 1.0 / marker_width;

    while (active())
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        double v = 1.0;

        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            marker_color = i == 0 ? WHITE : hsv2rgb(color, 1.0, v);
            set_pixel(np, (pos - i) % pixels, marker_color);
            v -= fade_step;
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
            color += 10;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + marker_width; pos++)
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        double v = 1.0;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                marker_color = i == 0 ? WHITE : hsv2rgb(color, 1.0, v);
                set_pixel(np, (pos - i) % pixels, marker_color);
                v -= fade_step;
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_comet_rainbow_trail_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);
    int marker_width = get_marker_width(np);

    int sweep = 0;
    int pos = 0;
    int step = 360 / (marker_width - 1);

    double seed = rand();

    while (active())
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            marker_color = i == 0 ? WHITE : hsv2rgb(seed - (i * step), 1.0, 1.0);
            set_pixel(np, (pos - i) % pixels, marker_color);
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + marker_width; pos++)
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                marker_color = i == 0 ? WHITE : hsv2rgb(seed + (i * step), 1.0, 1.0);
                set_pixel(np, (pos - i) % pixels, marker_color);
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_comet_rainbow_reveal_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);
    int marker_width = get_marker_width(np);

    int sweep = 0;
    int pos = 0;
    int step = 360 / pixels;

    double seed = rand() % 360;
    double fade_step = 1.0 / marker_width;

    while (active())
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        double v = 1.0;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            int idx = (pos - i) % pixels;
            marker_color = i == 0 ? WHITE : hsv2rgb(seed + (idx * step), 1.0, v);
            set_pixel(np, idx, marker_color);
            v -= fade_step;
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + marker_width; pos++)
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        int marker_color;
        double v = 1.0;
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                int idx = (pos - i) % pixels;
                marker_color = i == 0 ? WHITE : hsv2rgb(seed + (idx * step), 1.0, v);
                set_pixel(np, idx, marker_color);
                v -= fade_step;
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_full_rainbow_reveal_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);
    int marker_width = 3;

    int sweep = 0;
    int pos = 0;
    int step = 360 / pixels;

    double seed = rand();

    while (active())
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            if (pos - i < 0)
                break;

            int idx = (pos - i) % pixels;
            set_pixel(np, idx, hsv2rgb(seed + (idx * step), 1.0, 1.0));
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            int idx = (pos - i) % pixels;
            set_pixel(np, idx, WHITE);
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;
        // FIXME: this creates a cool counter rotation effect of the colors
        // seed += 2;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + pixels; pos++)
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            int idx = (pos - i) % pixels;

            if ((pos - i) < 0)
                break;

            if ((pos - i) >= end)
            {
                // If past the "end", cycle but draw black
                set_pixel(np, idx, 0);
            }
            else
            {
                // Otherwise, color the pixel
                set_pixel(np, idx, hsv2rgb(seed + (idx * step), 1.0, 1.0));
            }
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                int idx = (pos - i) % pixels;
                set_pixel(np, idx, WHITE);
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_full_color_dial(ws2811_t* np, active_func active)
{
    int marker_width = 3;
    int color = hsv2rgb(rand(), 1.0, 1.0);

    int pixels = num_pixels(np);

    int sweep = 0;
    int pos = 0;

    while (active())
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, i, color);
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, (pos - i) % pixels, WHITE);
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + pixels; pos++)
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            int idx = (pos - i) % pixels;

            if ((pos - i) < 0)
                break;

            if ((pos - i) >= end)
            {
                // If past the "end", cycle but draw black
                set_pixel(np, idx, 0);
            }
            else
            {
                // Otherwise, color the pixel
                set_pixel(np, idx, color);
            }
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                int idx = (pos - i) % pixels;
                set_pixel(np, idx, WHITE);
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_full_rainbow_wipe_dial(ws2811_t* np, active_func active)
{
    int marker_width = 3;

    int pixels = num_pixels(np);

    int sweep = 0;
    int pos = 0;

    double seed = rand();
    int background = hsv2rgb(seed, 1.0, 1.0);

    while (active())
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, i, background);
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, (pos - i) % pixels, WHITE);
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            background = hsv2rgb(seed + ((sweep + 1) * 5), 1.0, 1.0);
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + pixels; pos++)
    {
        // Draw the background pixels
        for (int i = 0; i < pixels; i++)
        {
            int idx = (pos - i) % pixels;

            if ((pos - i) < 0)
                break;

            if ((pos - i) >= end)
            {
                // If past the "end", cycle but draw black
                set_pixel(np, idx, 0);
            }
            else
            {
                // Otherwise, color the pixel
                set_pixel(np, idx, background);
            }
        }

        // Now set the marker
        for (int i = 0; i < marker_width; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                int idx = (pos - i) % pixels;
                set_pixel(np, idx, WHITE);
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void _effect_fire_ring(ws2811_t* np, int color_min, active_func active)
{
    int pixels = num_pixels(np);
    int colors[pixels];
    int dirs[pixels];
    int v[pixels];

    int color_max = color_min + 10;
    int color_mod = color_max - color_min;
    int min_v = 10;

    for (int i = 0; i < pixels; i++)
    {
        colors[i] = color_min + (rand() % color_mod);
        dirs[i] = colors[i] < color_max ? 1 : -1;
        v[i] = MAX(min_v, rand() % 100);

        set_pixel(np, i, hsv2rgb(colors[i], 1.0, v[i] / 100.0));

        ws2811_render(np);
        usleep(TICK);
    }

    while (active())
    {
        int delta = color_max - color_min;
        int phase_ms = 1000 * (1.0 / delta);

        for (int p = 0; p < delta; p++)
        {
            for (int i = 0; i < pixels; i++)
            {
                // Should we even change?
                if (rand() % 15 != 0) continue;

                colors[i] += dirs[i] + (dirs[i] * (rand() % (color_mod / 3)));
                v[i] += (-1 * dirs[i]) * (rand() % 25);

                if (v[i] > 100)
                    v[i] = 100;
                else if (v[i] < min_v)
                    v[i] = min_v;

                if (colors[i] >= color_max)
                {
                    dirs[i] = -1;
                    colors[i] = color_max;
                }
                else if (colors[i] <= color_min)
                {
                    dirs[i] = 1;
                    colors[i] = color_min;
                }

                set_pixel(np, i, hsv2rgb(colors[i], 1.0, v[i] / 100.0));
            }

            ws2811_render(np);
            usleep(phase_ms * 50);
        }

        ws2811_render(np);
        usleep(TICK);
    }

    // cleanup
    for (int i = 0; i < pixels; i++)
    {
        set_pixel(np, i, 0);
        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_fire_ring(ws2811_t* np, active_func active)
{
    _effect_fire_ring(np, 3, active);
}

void effect_random_fire_ring(ws2811_t* np, active_func active)
{
    _effect_fire_ring(np, rand() % 360, active);
}

void effect_unicorn_dial(ws2811_t* np, active_func active)
{
    int pixels = num_pixels(np);

    int sweep = 0;
    int pos = 0;
    int step = 360 / (pixels - 1);

    double seed = rand();

    while (active())
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        for (int i = 0; i < pixels; i++)
        {
            if (pos - i < 0)
                break;

            set_pixel(np, (pos - i) % pixels, hsv2rgb(seed + (i * step), 1.0, 1.0));
        }

        ws2811_render(np);
        usleep(TICK);

        pos++;

        if ((pos % pixels) == 0)
        {
            sweep++;
        }
    }

    // Cleanup animation, but adjust where the last position was and bound it
    int end = pos + (pixels - (pos % pixels));

    for (; pos < end + pixels; pos++)
    {
        // Draw all off pixels except the marker
        set_all_pixels(np, 0);

        // Now set the marker
        for (int i = 0; i < pixels; i++)
        {
            if (pos - i < 0)
                break;

            if ((pos - i) < end)
            {
                set_pixel(np, (pos - i) % pixels, hsv2rgb(seed + (i * step), 1.0, 1.0));
            }
        }

        ws2811_render(np);
        usleep(TICK);
    }
}

void _effect_strobe(ws2811_t* np, int on)
{
    int off = 0;
    int pixels = num_pixels(np);
    int strobes = 0;

    // Clear it
    set_all_pixels(np, off);

    // FIXME: configurable limit via parameter
    while (strobes < STROBE_MAX)
    {
        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, on);
        ws2811_render(np);
        usleep(STROBE_TICK);

        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, off);
        ws2811_render(np);
        usleep(STROBE_TICK);

        strobes++;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void effect_strobe(ws2811_t* np)
{
    _effect_strobe(np, hsv2rgb(0, 0, 1.0));
}

void effect_random_strobe(ws2811_t* np)
{
    _effect_strobe(np, hsv2rgb(rand(), 1.0, 1.0));
}

void effect_rainbow_strobe(ws2811_t* np)
{
    int off = 0;
    int pixels = num_pixels(np);
    int strobes = 0;
    int seed = rand();

    // Clear it
    set_all_pixels(np, off);

    // FIXME: configurable thing
    while (strobes < STROBE_MAX)
    {
        int on = hsv2rgb(seed, 1.0, 1.0);

        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, on);
        ws2811_render(np);
        usleep(STROBE_TICK);

        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, off);
        ws2811_render(np);
        usleep(STROBE_TICK);

        strobes++;
        seed += 10;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void effect_rainbow_static_strobe(ws2811_t* np)
{
    int off = 0;
    int pixels = num_pixels(np);
    int strobes = 0;
    int seed = rand();
    int step = 360 / pixels;

    // Clear it
    set_all_pixels(np, off);

    // FIXME: configurable
    while (strobes < STROBE_MAX)
    {
        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, hsv2rgb(seed + (i * step), 1.0, 1.0));
        ws2811_render(np);
        usleep(STROBE_TICK);

        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, off);
        ws2811_render(np);
        usleep(STROBE_TICK);

        strobes++;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void effect_rainbow_dynamic_strobe(ws2811_t* np)
{
    int off = 0;
    int pixels = num_pixels(np);
    int strobes = 0;
    int seed = rand();
    int step = 360 / pixels;

    // Clear it
    set_all_pixels(np, off);

    // FIXME configurable
    while (strobes < STROBE_MAX)
    {
        // Subtracting from seed makes the color look like its going clockwise
        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, hsv2rgb(seed - (i * step), 1.0, 1.0));
        ws2811_render(np);
        usleep(STROBE_TICK);

        for (int i = 0; i < pixels; i++)
            set_pixel(np, i, off);
        ws2811_render(np);
        usleep(STROBE_TICK);

        strobes++;
        seed += 25;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void _effect_twinkle(ws2811_t* np, int fg, int bg)
{
    int pixels = num_pixels(np);
    int sleep_count = 0;

    // FIXME configurable
    while (sleep_count < 5000000)
    {
        for (int i = 0; i < pixels; i++)
        {
            // Set some random guys
            if (rand() % (pixels / TWINKLE_SPARSE_FACTOR) == 0)
                set_pixel(np, i, fg);
            else
                set_pixel(np, i, bg);
        }

        ws2811_render(np);
        usleep(TWINKLE_TICK);
        sleep_count += TWINKLE_TICK;
    }

    // cleanup
    for (int i = 0; i < pixels; i++)
    {
        set_pixel(np, i, 0);
        ws2811_render(np);
        usleep(TICK);
    }
}

void effect_twinkle(ws2811_t* np)
{
    int c = rand();
    int fg = hsv2rgb(c, 1.0, 1.0);
    int bg = 0;

    _effect_twinkle(np, fg, bg);
}

void effect_rainbow_random_twinkle(ws2811_t* np)
{
    int off = 0;
    int pixels = num_pixels(np);
    int sleep_count = 0;
    int c = rand();

    // FIXME configurable
    while (sleep_count < 5000000)
    {
        // Clear first
        set_all_pixels(np, off);

        // Set some random guys to a color
        for (int i = 0; i < pixels; i++)
        {
            if (rand() % (pixels / TWINKLE_SPARSE_FACTOR) == 0)
                set_pixel(np, i, hsv2rgb(c + rand(), 1.0, 1.0));
        }
        ws2811_render(np);
        usleep(TWINKLE_TICK);
        sleep_count += TWINKLE_TICK;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void effect_rainbow_fixed_twinkle(ws2811_t* np)
{
    int off = 0;
    int pixels = num_pixels(np);
    int sleep_count = 0;
    int seed = rand() % 360;
    int step = 360 / pixels;

    // FIXME: configurable
    while (sleep_count < 5000000)
    {
        // Clear first
        set_all_pixels(np, off);

        // Set some random guys to a color
        for (int i = 0; i < pixels; i++)
        {
            if (rand() % (pixels / TWINKLE_SPARSE_FACTOR) == 0)
                set_pixel(np, i, hsv2rgb(seed + (i * step), 1.0, 1.0));
        }
        ws2811_render(np);
        usleep(TWINKLE_TICK);
        sleep_count += TWINKLE_TICK;
    }

    // cleanup
    set_all_pixels(np, off);
    ws2811_render(np);
    usleep(TICK);
}

void effect_dial_digit_highlight(ws2811_t* np, int digit)
{
    int pixels = num_pixels(np);

    // Run this effect a 2x speed from most animations
    int tick = TICK / 2;

    // About 40 degrees between the dialer stop and number 1
    // FIXME: this works in testing, but it needs to be less hardcoded
    int degree_start = 75;
    int degree_step = 32;

    // Update for a digit index from where we start (1 == 0 idx, 2 == 1, etc)
    // Digit 0 is special, it needs to be converted to index 9
    int digit_idx = digit == 0 ? 9 : digit - 1;

    // Now figure out how many pixels to light up.
    double pct_active = (degree_start + (digit_idx * degree_step)) / 360.0;
    int active_pixels = pixels * pct_active;

    // Now draw, but from the end
    int c = rgb2int(255, 255, 255);
    for (int i = 1; i <= active_pixels; i++)
    {
        set_pixel(np, pixels-i, c);
        ws2811_render(np);
        usleep(tick);
    }

    // pause for 500ms, then clear
    usleep(500000);

    for (int i = 0; i < pixels; i++)
    {
        set_pixel(np, i, 0);
        ws2811_render(np);
        usleep(tick);
    }
}

void random_sweep_effect(ws2811_t* np, active_func active)
{
    int n = 11;
    void (*effects[n]) (ws2811_t*, active_func);

    effects[0] = effect_unicorn_dial;
    effects[1] = effect_comet_dial;
    effects[2] = effect_comet_color_cycle_dial;
    effects[3] = effect_comet_rainbow_trail_dial;
    effects[4] = effect_comet_rainbow_reveal_dial;
    effects[5] = effect_full_rainbow_reveal_dial;
    effects[6] = effect_full_color_dial;
    effects[7] = effect_full_rainbow_wipe_dial;
    effects[8] = effect_fire_ring;
    effects[9] = effect_random_fire_ring;
    effects[10] = random_sweep_effect;

    effects[rand() % n](np, active);
}
