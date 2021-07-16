/*

Manage UART: implement stdio and fetch GPS data.

Copyright (C) 2019, ENSIMAG students
This project is under the MIT license

*/

#if GPS == 1

#include "app.h"
#include "gps.h"

#include <periph/uart.h>
#include <xtimer.h>

#include <stdio.h>
#include <string.h>

#define ENABLE_DEBUG (0)


// UART configuration.
#define STD_DEV      UART_DEV(0)

#ifndef STD_BAUDRATE
#define STD_BAUDRATE 9600
#endif

// Debug a GPS data.
#define DEBUG(...) if (ENABLE_DEBUG) printf(__VA_ARGS__)


// Unique instance of UART info structure.
uart_info_t uart_info;


// Handle interruption from UART.
static void uart_isr(uart_info_t *info, char c)
{
    if (info->line_length >= 127)
        info->line_length = 0;

    if (c != '$')
        goto store_c;
    if (strncmp(info->line, "$GPGGA", 6) != 0)
        goto reset_line;

    gps_parse_data((int8_t *)info->line, info->line_length);
    DEBUG("[uart] gps data: lat = %ld, lon = %ld, alt = %d\n",
        gps_data.latitude_bin, gps_data.longitude_bin, gps_data.altitude);

reset_line:
    info->line_length = 0;
store_c:
    info->line[info->line_length++] = c;
}


// STDIN is disabled in our application.
ssize_t stdio_read(void *buffer, size_t count)
{
    (void)buffer;  (void)count;
    return 0;
}

// Write STDOUT data to serial port 0.
ssize_t stdio_write(const void *buffer, size_t len)
{
    uart_write(STD_DEV, (const uint8_t *)buffer, len);
    return len;
}

// Initialize STDIO module.
void stdio_init(void)
{
    uart_init(STD_DEV, STD_BAUDRATE, (uart_rx_cb_t)uart_isr, &uart_info);
}

#endif
