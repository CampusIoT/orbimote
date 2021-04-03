/*
 * Copyright (C) 2020-2021 Université Grenoble Alpes
 */

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "xtimer.h"
#include <string.h>
#include "periph/wdt.h"


#ifndef WDT_UTILS_KICK_PERIOD
#define WDT_UTILS_KICK_PERIOD		4	  // sec
#endif

#ifndef WDT_UTILS_TIMEOUT
#define WDT_UTILS_TIMEOUT			10000 // msec
#endif


#ifndef THREAD_STACKSIZE_WDT
#define THREAD_STACKSIZE_WDT			THREAD_STACKSIZE_DEFAULT
#endif


// TODO mutex for started
static bool started = false;

static char wdt_thread_stack[THREAD_STACKSIZE_WDT]; // (THREAD_STACKSIZE_DEFAULT + THREAD_EXTRA_STACKSIZE_PRINTF)


static void *wdt_thread_func(void *arg){
	(void)(arg);

	puts("WDT started");
	started = true;
	wdt_setup_reboot(0, WDT_UTILS_TIMEOUT);
    wdt_start();
	while(started) {
		//puts("WDT kicked");
	    wdt_kick();
	    xtimer_sleep(WDT_UTILS_KICK_PERIOD);
	}
	#if WDT_HAS_STOP
	puts("WDT stopped");
	wdt_stop();
	#endif
	// for preventing several "wdt stop"
	started = false;

	return NULL;
}

/*
 * @brief wdt command
 *
 * @param argc
 * @param argv
 */
int wdt_cmd(int argc, char *argv[]) {
	if (argc == 2) {

		if (strstr(argv[1], "start") != NULL) {
			if(started) {
				puts("WDT already started");
				return -1;
			} else {
			    /* start the wdt thread */
			    thread_create(wdt_thread_stack, sizeof(wdt_thread_stack),
			                  THREAD_PRIORITY_MAIN - 1, 0, wdt_thread_func, NULL, "WDT");
				return 0;
			}
		}

#if WDT_HAS_STOP
		if (strstr(argv[1], "stop") != NULL) {
			if(!started) {
				puts("WDT already stopped");
				return -1;
			} else {
				puts("WDT stopping");
				started = false;
				return 0;
			}
		}
#endif
	}

#if WDT_HAS_STOP
	puts("usage: wdt <start|stop>");
#else
	puts("usage: wdt <start>");
#endif
	return -1;
}
