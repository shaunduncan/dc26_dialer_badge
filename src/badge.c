#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include <wiringPi.h>
#include "dialer.h"

void sighandler(int sig)
{
    printf("Bye!\n");

    // Kill everything
    stop_dialer();
}

void dial_cb(int digit)
{
    printf("Dialed: %d\n", digit);
}

int main()
{
    // Setup the signal handler
    struct sigaction sa = {
        .sa_handler = sighandler,
    };
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Run the dialer
    return run_dialer(dial_cb);
}
