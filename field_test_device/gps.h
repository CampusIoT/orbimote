/*

Decode GPS data.

Copyright (C) 2019, ENSIMAG students
This project is under the MIT license

*/


#pragma once

#include <stdint.h>
#include <stdbool.h>


#if GPS == 1

// Return codes.
#define GPS_SUCCESS  0
#define GPS_FAIL     1


// Store the GPS parsed data in ASCII.
typedef struct {
    char data_type[6];
    char latitude[10];
    char latitude_pole[2];
    char longitude[11];
    char longitude_pole[2];
    char fix_quality[2];
    char altitude[8];
} gps_nmea_t;


// Store GPS data.
typedef struct {
    bool has_fix;  // Ara data fixed?
    double latitude;
    double longitude;
    int32_t latitude_bin;
    int32_t longitude_bin;
    int16_t altitude;
} gps_data_t;

// GPS parsed data.
extern gps_data_t gps_data;


/**
 * @brief Get the lastest GPS position in binary format.
 * @param lat Where to store the latitude.
 * @param lon Where to store the longitude.
 * @param alt Where to store the altitude (in m).
 * @return Either `GPS_SUCCESS` or `GPS_FAIL`.
 */
uint8_t gps_get_binary(int32_t *lat, int32_t *lon, int16_t *alt);


/**
 * @brief Parse GPS data.
 * @param rxBuffer GPS data to parse.
 * @param rxBufferSize Length of data.
 * @return Either `GPS_SUCCESS` or `GPS_FAIL`.
 */
uint8_t gps_parse_data(int8_t *rxBuffer, int32_t rxBufferSize);

/**
 * @brief Reset parsed GPS data.
 */
void gps_reset_data(void);

#endif
