
#include <stddef.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#include <time.h>

#include "periph_conf.h"
#include "periph/rtc.h"

#include <inttypes.h>

#define TM_YEAR_OFFSET      (1900)

const uint8_t _ytab[2][12] = {
                { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
        };

#define YEAR0           1900                    /* the first year */
#define EPOCH_YR        1970            /* EPOCH = Jan 1 1970 00:00:00 */
#define SECS_DAY        (24L * 60L * 60L)
#define LEAPYEAR(year)  (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)  (LEAPYEAR(year) ? 366 : 365)

/*
 * convert epoch (in seconds) into a tm struct
 * epoch_to_time is inspired from gmtime
 */
/* $Header: /opt/proj/minix/cvsroot/src/lib/ansi/gmtime.c,v 1.1.1.1 2005/04/21 14:56:05 beng Exp $ */

void epoch_to_time(struct tm *timep, const uint32_t epoch)
{
        register unsigned long dayclock, dayno;
        int year = EPOCH_YR;

        dayclock = (unsigned long)epoch % SECS_DAY;
        dayno = (unsigned long)epoch / SECS_DAY;

        timep->tm_sec = dayclock % 60;
        timep->tm_min = (dayclock % 3600) / 60;
        timep->tm_hour = dayclock / 3600;
        timep->tm_wday = (dayno + 4) % 7;       /* day 0 was a thursday */
        while (dayno >= YEARSIZE(year)) {
                dayno -= YEARSIZE(year);
                year++;
        }
        timep->tm_year = year - YEAR0;
        timep->tm_yday = dayno;
        timep->tm_mon = 0;
        while (dayno >= _ytab[LEAPYEAR(year)][timep->tm_mon]) {
                dayno -= _ytab[LEAPYEAR(year)][timep->tm_mon];
                timep->tm_mon++;
        }
        timep->tm_mday = dayno + 1;
        timep->tm_isdst = 0;
}

/*
 * print a tm struct
 */
void print_time(const char *label, const struct tm *time)
{
    DEBUG("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
            time->tm_year + TM_YEAR_OFFSET,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec);
}
