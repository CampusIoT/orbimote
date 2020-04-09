
#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void epoch_to_time(struct tm *timep, const uint32_t epoch);

    void print_time(const char *label, const struct tm *time);

#endif