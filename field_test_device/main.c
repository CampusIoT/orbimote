/*
 * Read pediodically the temperature from the DS75LX sensor
 * then send to the LoRaWAN network in which the endpoint is registered.
 * The payload format is LPP Cayenne.
 * 
 * Copyright (C) 2020 LIG Université Grenoble Alpes
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#define ENABLE_DEBUG (1)
#include "debug.h"

#include <string.h>

#include "xtimer.h"
#include <time.h>

#include "mutex.h"
#include "periph_conf.h"
#include "periph/rtc.h"

#include "cpu_conf.h"
#include "periph/cpuid.h"

#include "board.h"

#include "fmt.h"

#include "net/loramac.h"
#include "semtech_loramac.h"
#include "loramac_utils.h"

#include "git_utils.h"
#include "wdt_utils.h"

#if DS75LX == 1
#include "ds75lx.h"
#include "ds75lx_params.h"
#endif

#if GPS == 1
#include "gps.h"
#endif

#include "app_clock.h"
#include "benchmark.h"

#include <random.h>

/* Declare globally the loramac descriptor */
static semtech_loramac_t loramac;

/* Declare globally the sensor device descriptor */
#if DS75LX == 1
ds75lx_t ds75lx;
#endif

// Count the number of elements in an array.
#define CNT(array) (uint8_t)(sizeof(array) / sizeof(*array))

/* LoRaMac values */
#define JOIN_NEXT_RETRY_TIME            10 // Next join tentative in 1 minute
#define SECONDS_PER_DAY                 24 * 60 * 60

/* Use a fast datarate, e.g. BW125/SF7 in EU868 */
#define DR_INIT                         LORAMAC_DR_5

#define FIRST_TX_PERIOD                 TXPERIOD
#define TX_PERIOD                       TXPERIOD

#define PORT_UP_DATA                    101
#define PORT_UP_ERROR                   102

#define PORT_DN_TEXT                    101
#define PORT_DN_SET_TX_PERIOD           3

#ifndef VIRT_DEV
#define VIRT_DEV 						(1U)
#endif


/* Implement the receiver thread */
#define RECEIVER_MSG_QUEUE                          (4U)

#if OTAA == 1

#ifdef FORGE_DEVEUI_APPEUI_APPKEY
static uint8_t secret[LORAMAC_APPKEY_LEN];
#endif

static uint8_t deveui[LORAMAC_DEVEUI_LEN] ;
static uint8_t appeui[LORAMAC_APPEUI_LEN] ;
static uint8_t appkey[LORAMAC_APPKEY_LEN] ;

#else

// static uint32_t* devaddr = { DEVADDRS };

static uint8_t devaddr[LORAMAC_DEVADDR_LEN] ;
static uint8_t appskey[LORAMAC_NWKSKEY_LEN] ;
static uint8_t nwkskey[LORAMAC_APPSKEY_LEN] ;

#endif

static msg_t _receiver_queue[RECEIVER_MSG_QUEUE];
static char _receiver_stack[THREAD_STACKSIZE_DEFAULT];

static uint16_t tx_period = TX_PERIOD;

static void init_sensors(void){

    uint8_t port = PORT_UP_DATA;

#if GPS == 1
    DEBUG("[gps] GPS is enabled (baudrate=%d)\n",STD_BAUDRATE);
#endif

#if DS75LX == 1
    DEBUG("[ds75lx] DS75LX sensor is enabled\n");

    int result = ds75lx_init(&ds75lx, &ds75lx_params[0]);
    if (result != DS75LX_OK)
    {
        DEBUG("[error] Failed to initialize DS75LX sensor\n");
        port = PORT_UP_ERROR;
    }
#endif

    semtech_loramac_set_tx_port(&loramac, port);
}

// Encode message data to the payload.
unsigned int encode_sensors(uint8_t *payload, const unsigned int len) {

	if(len < sizeof(int16_t)) {
		return 0;
	}

	int16_t temperature = 0;

#if DS75LX == 1
    /* measure temperature */
    ds75lx_wakeup(&ds75lx);
    /* Get temperature in degrees celsius */
    ds75lx_read_temperature(&ds75lx, &temperature);
    ds75lx_shutdown(&ds75lx);
    DEBUG("[ds75lx] get temperature : temperature=%d\n",temperature);

#endif

    unsigned int i = 0;

    // Encode temperature.
	payload[i++] = (temperature >> 8) & 0xFF;
	payload[i++] = (temperature >> 0) & 0xFF;

	if(len < sizeof(int16_t) + (2*3)+ sizeof(int16_t)) {
		return sizeof(int16_t);
	}

	int32_t lat = 0;
	int32_t lon = 0;
	int16_t alt = 0;

#if GPS == 1
	gps_get_binary(&lat, &lon, &alt);
    DEBUG("[gps] get position : lat=%ld, lon=%ld, alt=%d\n",lat,lon,alt);
#endif

    // Encode latitude (on 24 bits).
	payload[i++] = ((uint32_t)lat >> 16) & 0xFF;
	payload[i++] = ((uint32_t)lat >> 8)  & 0xFF;
	payload[i++] = ((uint32_t)lat >> 0)  & 0xFF;

    // Encode longitude (on 24 bits).
	payload[i++] = ((uint32_t)lon >> 16) & 0xFF;
	payload[i++] = ((uint32_t)lon >> 8)  & 0xFF;
	payload[i++] = ((uint32_t)lon >> 0)  & 0xFF;

    // Encode altitude (on 16 bits);
	payload[i++] = ((int16_t)alt >> 8) & 0xFF;
	payload[i++] = ((int16_t)alt >> 0) & 0xFF;

	return sizeof(int16_t) + (2*3)+ sizeof(int16_t);
}


static void sender(void)
{
	// request for clock synchronization
    semtech_loramac_set_tx_mode(&loramac, LORAMAC_TX_UNCNF);

	app_clock_send_app_time_req(&loramac);

    xtimer_sleep(tx_period);

    uint8_t drpwsz_sequence[] = { DRPWSZ_SEQUENCE };
    struct benchmark_t benchmark;
    semtech_loramac_get_devaddr(&loramac, (uint8_t*)&benchmark.devaddr);
    benchmark.nb_virtual_devices = VIRT_DEV;
    benchmark.tx_period = &tx_period;
    benchmark.drpwsz_sequence_nb = CNT(drpwsz_sequence) / 3;
    benchmark.drpwsz_sequence = drpwsz_sequence;
    benchmark.txconfirmed = TXCNF;
    benchmark.adr = ADR_ON;
    benchmark.min_port = MIN_PORT;
    benchmark.max_port = MAX_PORT;

    benchmark_start(&loramac, benchmark, encode_sensors);

    /* this should never be reached */
    return;
}

static void *receiver(void *arg)
{
    msg_init_queue(_receiver_queue, RECEIVER_MSG_QUEUE);

    (void)arg;
    while (1) {
        app_clock_print_rtc();

        /* blocks until something is received */
        switch (semtech_loramac_recv(&loramac)) {
            case SEMTECH_LORAMAC_RX_DATA:
                // TODO process Downlink payload
                switch(loramac.rx_data.port) {
                    case PORT_DN_TEXT:
                        loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
                        DEBUG("[dn] Data received: text=%s, port: %d\n",
                            (char *)loramac.rx_data.payload, loramac.rx_data.port);
                        break;
                    case PORT_DN_SET_TX_PERIOD:
                        if(loramac.rx_data.payload_len == sizeof(tx_period)) {
                            tx_period=*((uint16_t*)loramac.rx_data.payload);
                            DEBUG("[dn] Data received: tx_period=%d, port: %d\n",
                                tx_period, loramac.rx_data.port);
                        } else {
                            DEBUG("[dn] Data received: bad size for tx_period, port: %d\n",
                                 loramac.rx_data.port);
                        }
                        break;
                    case APP_CLOCK_PORT:
                    	(void)app_clock_process_downlink(&loramac);
                    	break;
                    default:
                        DEBUG("[dn] Data received: ");
                        printf_ba(loramac.rx_data.payload, loramac.rx_data.payload_len);
                        DEBUG(", port: %d\n",loramac.rx_data.port);
                        break;
                }
                break;

            case SEMTECH_LORAMAC_RX_CONFIRMED:
                // TODO if too much unconfirmed Tx frames --> rejoin
                DEBUG("[dn] Received ACK from network\n");
                break;

            default:
                break;
        }
    }
    return NULL;
}

static void cpuid_info(void) {
	uint8_t id[CPUID_LEN];
	/* read the CPUID */
	cpuid_get(id);
	DEBUG("[info] CpuId:"); printf_ba(id,CPUID_LEN); DEBUG("\n");
}

static void loramac_info(void) {
#ifdef OPERATOR
    DEBUG("[info] Operator: %s\n", OPERATOR);
#endif
#ifdef LABEL
    DEBUG("[info] Label: %s\n",LABEL);
#endif
    DEBUG("[mac] Region: " LORAMAC_REGION_STR "\n");
#if EU868_DUTY_CYCLE_ENABLED == 0
    DEBUG("[mac] DutyCycle: disabled\n");
#else
    DEBUG("[mac] DutyCycle: enabled\n");
#endif
}

static char* wdt_cmdline[] = {"wdt","start"};

int main(void)
{

	git_cmd(0, NULL);
	wdt_cmd(2, wdt_cmdline);


    app_clock_print_rtc();

    cpuid_info();
    loramac_info();

    /* initialize the sensors */
    init_sensors();

    /* initialize the loramac stack */
    semtech_loramac_init(&loramac);

#if OTAA == 1

#ifdef FORGE_DEVEUI_APPEUI_APPKEY
    /* forge the deveui, appeui and appkey of the endpoint */
    fmt_hex_bytes(secret, SECRET);
    loramac_utils_forge_euis_and_key(deveui,appeui,appkey,secret);
    DEBUG("[otaa] Secret:"); printf_ba(secret,LORAMAC_APPKEY_LEN); DEBUG("\n");
#else
    /* Convert identifiers and application key */
    fmt_hex_bytes(deveui, DEVEUI);
    fmt_hex_bytes(appeui, APPEUI);
    fmt_hex_bytes(appkey, APPKEY);
#endif
	DEBUG("[otaa] DevEUI:"); printf_ba(deveui,LORAMAC_DEVEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppEUI:"); printf_ba(appeui,LORAMAC_APPEUI_LEN); DEBUG("\n");
    DEBUG("[otaa] AppKey:"); printf_ba(appkey,LORAMAC_APPKEY_LEN); DEBUG("\n");

    /* set the LoRaWAN keys */
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* start the OTAA join procedure (and retries in required) */
    /*uint8_t joinRes = */ loramac_utils_join_retry_loop(&loramac, DR_INIT, JOIN_NEXT_RETRY_TIME, SECONDS_PER_DAY);

	random_init_by_array((uint32_t*)appkey, LORAMAC_APPKEY_LEN/4);

#else
    /* Convert identifiers and application key */

	// uint32_t devaddr = devaddrs[0];

    fmt_hex_bytes(devaddr, DEVADDR);
    fmt_hex_bytes(appskey, APPSKEY);
    fmt_hex_bytes(nwkskey, NWKSKEY);

    DEBUG("[abp] DevAddr:"); printf_ba(devaddr,LORAMAC_DEVADDR_LEN); DEBUG("\n");
    DEBUG("[abp] AppSKey:"); printf_ba(appskey,LORAMAC_NWKSKEY_LEN); DEBUG("\n");
    DEBUG("[abp] NwkSKey:"); printf_ba(nwkskey,LORAMAC_APPSKEY_LEN); DEBUG("\n");

    /* set the LoRaWAN keys */
    semtech_loramac_set_devaddr(&loramac, devaddr);
    semtech_loramac_set_appskey(&loramac, appskey);
    semtech_loramac_set_nwkskey(&loramac, nwkskey);


    /* start the ABP join procedure (and retries in required) */
    /*uint8_t joinRes = */ loramac_utils_abp_join_retry_loop(&loramac, DR_INIT, JOIN_NEXT_RETRY_TIME, SECONDS_PER_DAY);

	random_init_by_array((uint32_t*)appskey, LORAMAC_APPSKEY_LEN/4);

#endif


    /* start the receiver thread */
    thread_create(_receiver_stack, sizeof(_receiver_stack),
                  THREAD_PRIORITY_MAIN - 1, 0, receiver, NULL, "RECEIVER");

    /* sleep FIRST_TX_PERIOD secs */
    xtimer_sleep(FIRST_TX_PERIOD);

    /* call the sender */
    sender();
    
    return 0; /* should never be reached */
}
