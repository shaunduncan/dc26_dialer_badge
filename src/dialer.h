#ifndef __DIALER_H__
#define __DIALER_H__

#include <stdbool.h>

// Callback function typedef for when we find a digit
typedef void (*dialer_cb_t)(int digit);

int run_dialer(dialer_cb_t cb);
void stop_dialer();

#endif
