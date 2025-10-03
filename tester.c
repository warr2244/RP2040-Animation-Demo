#include <stdio.h>
#include "pico/stdlib.h"
#include "pico-usb/pio_usb.h"

int main()
{
    stdio_init_all();

    while (true) {
        stdio_printf("Hello, World!");
        sleep_ms(1000);
    }
}
