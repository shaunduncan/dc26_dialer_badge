#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <wiringPi.h>
#include <ws2811.h>

#include "dialer.h"
#include "effects.h"

#define DIALER_CONTROL_PIN 2
#define DIALER_SIGNAL_PIN 3
#define DIALER_COUNT_LOCK 0
#define DIALER_SWEEP_LOCK 1
#define DIALER_DIGITS_LOCK 2

#define DIAL_OFF HIGH
#define DIAL_ON LOW

#define LED_SIGNAL_PIN 21
#define LED_DEFAULT_BRIGHTNESS 50
#define LED_NUM_PIXELS 43
#define LED_FREQ_HZ 1000000


// Global state
bool running = true;
bool dial_sweep_active = false;

// State for the actual dial
int pulse_count = 0;
int dial_start;
int* digits;
int num_digits;
int digits_idx;

// Neopixel struct
ws2811_t* np = NULL;

// Separate thread to run the sweep effect while we count pulses
pthread_t lighting_thread;


// ISR callback for the signal pin
void on_signal_pulse()
{
    // FIXME: should we deactivate the dial sweep here?
    piLock(DIALER_COUNT_LOCK);
    pulse_count++;
    piUnlock(DIALER_COUNT_LOCK);
}

void reset_pulse_count()
{
    piLock(DIALER_COUNT_LOCK);
    pulse_count = 0;
    piUnlock(DIALER_COUNT_LOCK);
}

void activate_dial_sweep()
{
    piLock(DIALER_SWEEP_LOCK);
    dial_sweep_active = true;
    piUnlock(DIALER_SWEEP_LOCK);
}

void deactivate_dial_sweep()
{
    piLock(DIALER_SWEEP_LOCK);
    dial_sweep_active = false;
    piUnlock(DIALER_SWEEP_LOCK);
}

bool is_dial_sweep_active()
{
    return dial_sweep_active;
}

int init_lighting()
{
    np = (ws2811_t*) malloc(sizeof(ws2811_t));
    if (np == NULL)
        return 1;

    // Initialize
    np->render_wait_time = 0;
    np->freq = LED_FREQ_HZ;
    np->dmanum = 10;
    np->channel[0] = (ws2811_channel_t) {
       .gpionum = LED_SIGNAL_PIN,
       .count = LED_NUM_PIXELS,
       .invert = 0,
       .brightness = LED_DEFAULT_BRIGHTNESS,
       .strip_type = WS2811_STRIP_GRB,
    };

    // Initialize and clear
    ws2811_init(np);
    effect_clear(np);

    return 0;
}

// pthread target
void *run_dial_sweep_effect(void* ptr)
{
    random_sweep_effect(np, is_dial_sweep_active);
    effect_clear(np);
    return ptr;
}

void cleanup_lighting()
{
    effect_clear(np);
    ws2811_fini(np);

    if (np != NULL)
        free(np);
}

int init_dialer()
{
    dial_start = -1;
    digits_idx = 0;
    num_digits = 32;
    digits = (int *) malloc(num_digits * sizeof(int));

    wiringPiSetupGpio();

    pinMode(DIALER_CONTROL_PIN, INPUT);
    pinMode(DIALER_SIGNAL_PIN, INPUT);

    // Setup the callback
    wiringPiISR(DIALER_SIGNAL_PIN, INT_EDGE_RISING, on_signal_pulse);

    return 0;
}

void store_digit(int digit)
{
    piLock(DIALER_DIGITS_LOCK);

    // Check for the need to expand
    if (digits_idx == num_digits-1)
    {
        //num_digits *= 2;
        //digits = (int*) realloc(num_digits * sizeof(int));
    }

    digits[digits_idx] = digit;
    digits_idx++;

    piUnlock(DIALER_DIGITS_LOCK);
}

void reset_digits()
{
    piLock(DIALER_DIGITS_LOCK);
    digits_idx = 0;
    //memset(digits, 0, num_digits * sizeof(int));
    piUnlock(DIALER_DIGITS_LOCK);
}

int run_dialer(dialer_cb_t dialer_cb)
{
    if (init_dialer() != 0)
    {
        printf("Failed to initialize dialer...\n");
        return 1;
    }

    // Initialize the lighting
    if (init_lighting() != 0)
    {
        printf("Failed to initialize lighting\n");
        return 1;
    }

    // Need a pthread target
    pthread_t effect_thread;
    deactivate_dial_sweep();

    printf("Ready for dial...\n");

    while (running)
    {
        while (digitalRead(DIALER_CONTROL_PIN) == DIAL_OFF && running)
        {
            // TODO: if this is idle for a long time, run a demo!
            delay(5);
            continue;
        }

        // Do we need to quit?
        if (!running)
            break;

        // Run the lighting thread if this is the first detection
        activate_dial_sweep();
        pthread_create(&effect_thread, NULL, run_dial_sweep_effect, NULL);

        // When the gate is low (open), we're dialing
        while (digitalRead(DIALER_CONTROL_PIN) == DIAL_ON)
        {
            delay(1);
            continue;
        }

        // Stop the dial sweep thread (todo: pthread_timedjoin_np)
        deactivate_dial_sweep();
        pthread_join(effect_thread, NULL);

        // If the pulse count was zero, treat it as an error
        if (pulse_count > 0)
        {
            int digit = (pulse_count / 2) % 10;

            dialer_cb(digit);
            effect_dial_digit_highlight(np, digit);
            printf("Ready for dial...\n");
        }
        reset_pulse_count();
    }

    return 0;
}

void stop_dialer()
{
    running = false;
    cleanup_lighting();
}
