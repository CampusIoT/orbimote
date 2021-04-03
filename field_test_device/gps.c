/*

Parse GPS data.

Copyright (C) 2019, ENSIMAG students
This file is based on LoRaMAC-node from Semtech, under Revised BSD License.

*/
#ifdef GPS

#include "gps.h"

#include <mutex.h>

#include <string.h>
#include <stdlib.h>


// Various type of NMEA data we can receive with the GPS.
static const char NmeaDataTypeGPGGA[] = "GPGGA";
static const char NmeaDataTypeGPRMC[] = "GPRMC";
// TODO process messages GPGLL : Latitude, longitude, UTC time of position fix and status.
// TODO process messages GPGSA : GPS receiver operating mode, satellites used in the position solution, and DOP values.
// TODO process messages GPGSV : The number of GPS satellites in view satellite ID numbers, elevation, azimuth, and SNR values.
// TODO process messages GPMSS : Signal-to-noise ratio, signal strength, frequency, and bit rate from a radio-beacon receiver.
// TODO process messages GPRMC : Time, date, position, course and speed data.
// TODO process messages GPVTG : Course and speed information relative to the ground.
// TODO process messages GPZDA : PPS timing message (synchronized to PPS).


// TODO process messages from BD ou GB - Beidou ; GA - Galileo ; GL - GLONASS.


// Value used for the conversion of the position from DMS to decimal.
static const int32_t MaxNorthPosition = 8388607;  // 2^23 - 1
static const int32_t MaxSouthPosition = 8388608;  // -2^23
static const int32_t MaxEastPosition  = 8388607;  // 2^23 - 1
static const int32_t MaxWestPosition  = 8388608;  // -2^23

// GPS data in ASCII and numrical formats.
gps_nmea_t gps_nmea;
gps_data_t gps_data;

// Mutex that protect GPS data.
static mutex_t gps_mutex = MUTEX_INIT;


// Convert a nibble to hex char.
static int8_t nibble_to_hex(uint8_t a)
{
    if (a < 10)
        return '0' + a;
    else if (a < 16)
        return 'A' + (a - 10);
    else
        return '?';
}



// Convert GPS positions from double to binary values.
static void positions_to_binary(void)
{
    long double temp;

    if (gps_data.latitude >= 0) { // North
        temp = gps_data.latitude * MaxNorthPosition;
        gps_data.latitude_bin = temp / 90;
    } else {                      // South
        temp = gps_data.latitude * MaxSouthPosition;
        gps_data.latitude_bin = temp / 90;
    }

    if (gps_data.longitude >= 0) { // East
        temp = gps_data.longitude * MaxEastPosition;
        gps_data.longitude_bin = temp / 180;
    } else {                       // West
        temp = gps_data.longitude * MaxWestPosition;
        gps_data.longitude_bin = temp / 180;
    }
}


// Convert GPS positions from ASCII to double values.
static void positions_to_double(void)
{
    double valueTmp1, valueTmp2, valueTmp3, valueTmp4;

    // Convert the latitude from ASCII to uint8_t values.
    for (int i = 0 ; i < 10 ; i++ )
        gps_nmea.latitude[i] = gps_nmea.latitude[i] & 0xF;

    // Convert latitude from degree/minute/second (DMS) format into decimal.
    valueTmp1 = (double)gps_nmea.latitude[0] * 10.0 + (double)gps_nmea.latitude[1];
    valueTmp2 = (double)gps_nmea.latitude[2] * 10.0 + (double)gps_nmea.latitude[3];
    valueTmp3 = (double)gps_nmea.latitude[5] * 1000.0 + (double)gps_nmea.latitude[6] * 100.0 +
                (double)gps_nmea.latitude[7] * 10.0 + (double)gps_nmea.latitude[8];

    gps_data.latitude = valueTmp1 + ((valueTmp2 + (valueTmp3 * 0.0001)) / 60.0);
    if (gps_nmea.latitude_pole[0] == 'S')
        gps_data.latitude *= -1;

    // Convert the longitude from ASCII to uint8_t values.
    for (int i = 0 ; i < 10 ; i++)
        gps_nmea.longitude[i] = gps_nmea.longitude[i] & 0xF;

    // Convert longitude from degree/minute/second (DMS) format into decimal.
    valueTmp1 = (double)gps_nmea.longitude[0] * 100.0 + (double)gps_nmea.longitude[1] * 10.0 + (double)gps_nmea.longitude[2];
    valueTmp2 = (double)gps_nmea.longitude[3] * 10.0 + (double)gps_nmea.longitude[4];
    valueTmp3 = (double)gps_nmea.longitude[6] * 1000.0 + (double)gps_nmea.longitude[7] * 100;
    valueTmp4 = (double)gps_nmea.longitude[8] * 10.0 + (double)gps_nmea.longitude[9];

    gps_data.longitude = valueTmp1 + (valueTmp2 / 60.0) + (((valueTmp3 + valueTmp4) * 0.0001) / 60.0);
    if (gps_nmea.longitude_pole[0] == 'W')
        gps_data.longitude *= -1;
}



// Calculates the checksum for a NMEA sentence (and return the position of the
// checksum in the sentence).
static int32_t nmea_checksum(int8_t *nmeaStr, int32_t nmeaStrSize, int8_t *checksum)
{
    int i = 0;
    uint8_t checkNum = 0;

    if ((nmeaStr == NULL) || (checksum == NULL) || (nmeaStrSize <= 1))
        return -1;

    if (nmeaStr[i] == '$')
        i += 1;  // Skip the first '$' if necessary.

    while (nmeaStr[i] != '*') {
        checkNum ^= nmeaStr[i];  // XOR until '*' or max length is reached.
        i += 1;
        if (i >= nmeaStrSize)
            return -1;
    }

    // Convert checksum value to 2 hexadecimal characters.
    checksum[0] = nibble_to_hex(checkNum / 16); // upper nibble
    checksum[1] = nibble_to_hex(checkNum % 16); // lower nibble
    return i + 1;
}


// Calculate the checksum of a NMEA frame and compare it to the checksum that
// is present at the end of it (return true if it matches).
static bool nmea_validate_checksum(int8_t *serialBuff, int32_t buffSize)
{
    int32_t checksumIndex;
    int8_t checksum[2]; // 2 characters to calculate NMEA checksum.

    checksumIndex = nmea_checksum(serialBuff, buffSize, checksum);
    if (checksumIndex < 0)
        return false;

    if (checksumIndex >= (buffSize - 2))
        return false;  // Not enough char in the serial buffer to read checksum.

    return  // Check the checksum.
        (serialBuff[checksumIndex + 0] == checksum[0]) &&
        (serialBuff[checksumIndex + 1] == checksum[1]);
}


// Format GPS data.
static void format_gps_data(void)
{
    positions_to_double();
    positions_to_binary();

    if (gps_data.has_fix)
        gps_data.altitude = atoi(gps_nmea.altitude);
}



// Read a field from RX buffer.
#define READ_FIELD(field, i, rxBuffer, maxSize)       \
{                                                     \
    uint8_t __fs = 0;                                 \
    while ((rxBuffer)[(i) + __fs++] != ',')           \
        if (__fs > (maxSize)) return GPS_FAIL;        \
    for (uint8_t __j = 0; __j < __fs; __j++, (i)++)   \
        (field)[__j] = (rxBuffer)[i];                 \
}

// Read a field from RX buffer.
#define SKIP_FIELD(i, rxBuffer, maxSize)              \
{                                                     \
    uint8_t __fs = 0;                                 \
    while ((rxBuffer)[(i) + __fs++] != ',')           \
        if (__fs > (maxSize)) return GPS_FAIL;        \
    (i) += __fs;                                      \
}


// Parse a GPGGA message.
static uint8_t parse_GPGGA(uint8_t i, int8_t *rxBuffer)
{
    // NmeaUtcTime.
    SKIP_FIELD(i, rxBuffer, 11);
    // NmeaLatitude.
    READ_FIELD(gps_nmea.latitude, i, rxBuffer, 10);
    // NmeaLatitudePole.
    READ_FIELD(gps_nmea.latitude_pole, i, rxBuffer, 2);
    // NmeaLongitude.
    READ_FIELD(gps_nmea.longitude, i, rxBuffer, 11);
    // NmeaLongitudePole.
    READ_FIELD(gps_nmea.longitude_pole, i, rxBuffer, 2);
    // NmeaFixQuality.
    READ_FIELD(gps_nmea.fix_quality, i, rxBuffer, 2);
    // NmeaSatelliteTracked.
    SKIP_FIELD(i, rxBuffer, 3);
    // NmeaHorizontalDilution.
    SKIP_FIELD(i, rxBuffer, 6);
    // NmeaAltitude.
    READ_FIELD(gps_nmea.altitude, i, rxBuffer, 8);
    // NmeaAltitudeUnit.
    SKIP_FIELD(i, rxBuffer, 2);
    // NmeaHeightGeoid.
    SKIP_FIELD(i, rxBuffer, 8);
    // NmeaHeightGeoidUnit.
    SKIP_FIELD(i, rxBuffer, 2);

    gps_data.has_fix = (gps_nmea.fix_quality[0] > 0x30);
    format_gps_data();

    return GPS_SUCCESS;
}


// Parse a GPRMC message.
static uint8_t parse_GPRMC(uint8_t i, int8_t *rxBuffer)
{
    // NmeaUtcTime.
    SKIP_FIELD(i, rxBuffer, 11);
    // NmeaDataStatus.
    READ_FIELD(gps_nmea.fix_quality, i, rxBuffer, 2);
    // NmeaLatitude.
    READ_FIELD(gps_nmea.latitude, i, rxBuffer, 10);
    // NmeaLatitudePole.
    READ_FIELD(gps_nmea.latitude_pole, i, rxBuffer, 2);
    // NmeaLongitude.
    READ_FIELD(gps_nmea.longitude, i, rxBuffer, 11);
    // NmeaLongitudePole.
    READ_FIELD(gps_nmea.longitude_pole, i, rxBuffer, 2);
    // NmeaSpeed.
    SKIP_FIELD(i, rxBuffer, 8);
    // NmeaDetectionAngle.
    SKIP_FIELD(i, rxBuffer, 8);
    // NmeaDate.
    SKIP_FIELD(i, rxBuffer, 8);

    gps_data.has_fix = (gps_nmea.fix_quality[0] == 0x41);
    format_gps_data();

    return GPS_SUCCESS;
}


// Parse GPS data.
uint8_t gps_parse_data(int8_t *rxBuffer, int32_t rxBufferSize)
{
    if (!nmea_validate_checksum(rxBuffer, rxBufferSize))
        return GPS_FAIL;

    uint8_t i = 1;
    READ_FIELD(gps_nmea.data_type, i, rxBuffer, 6);

    if (strncmp(gps_nmea.data_type, NmeaDataTypeGPGGA, 5) == 0)
        return parse_GPGGA(i, rxBuffer);
    else if (strncmp(gps_nmea.data_type, NmeaDataTypeGPRMC, 5) == 0)
        return parse_GPRMC(i, rxBuffer);
    else
        return GPS_FAIL;
}


// Get the lastest GPS position in binary format.
uint8_t gps_get_binary(int32_t *lat, int32_t *lon, int16_t *alt)
{
    mutex_lock(&gps_mutex);

    uint8_t status = gps_data.has_fix ? GPS_SUCCESS : GPS_FAIL;
    if (!gps_data.has_fix)
        gps_reset_data();

    *lat = gps_data.latitude_bin;
    *lon = gps_data.longitude_bin;
    *alt = gps_data.altitude;

    mutex_unlock(&gps_mutex);
    return status;
}


// Reset GPS data.
void gps_reset_data(void)
{
    gps_data.has_fix = false;
    gps_data.altitude = 0xFFFF;

    gps_data.latitude = 0;
    gps_data.longitude = 0;

    gps_data.latitude_bin = 0;
    gps_data.longitude_bin = 0;
}

#endif
