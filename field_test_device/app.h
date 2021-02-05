/*

Cubsat benchmark application.

Copyright (C) 2019, ENSIMAG students
This project is under the MIT license

*/
#ifdef GPS

#include <mutex.h>
#include <panic.h>

#include <stdint.h>
#include <stdbool.h>


// Stop the application on an error.
#define PANIC(msg) core_panic(PANIC_GENERAL_ERROR, msg)


// Update UART line every .. ms
#define UART_UPDATE_MS  500

// Store information given by UART.
typedef struct {
    mutex_t mutex;
    bool is_parsing;
    uint8_t line_length;
    char line[128];
} uart_info_t;

// Unique instance of UART info structure.
extern uart_info_t uart_info;

#endif
