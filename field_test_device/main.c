/*
 * Read pediodically the temperature from the DS75LX sensor
 * then send to the LoRaWAN network in which the endpoint is registered.
 * The payload format is LPP Cayenne.
 * 
 * Copyright (C) 2020-2022 LIG Université Grenoble Alpes
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * @author Didier DONSEZ
 */

#define ENABLE_DEBUG (1)
#include "debug.h"

#include <string.h>

#include "xtimer.h"
#include <time.h>

#include "mutex.h"
#include "periph_conf.h"
#include "periph/rtc.h"
#include "periph/pm.h"

#include "cpu_conf.h"
#include "periph/cpuid.h"

#include "board.h"

#include "fmt.h"

//#include "net/loramac.h"
#include "semtech_loramac.h"
#include "loramac_utils.h"

#include "git_utils.h"
#include "wdt_utils.h"

#if DS75LX == 1
#include "ds75lx.h"
#include "ds75lx_params.h"
#endif

#if AT30TES75X == 1
#include "at30tse75x.h"
#endif

#if MODULE_MAG3110 == 1
#include "mag3110.h"
#include "mag3110_params.h"
#endif

#if MODULE_MMA8X5X == 1
#include "mma8x5x.h"
#include "mma8x5x_params.h"
#endif

#if MODULE_MPL3115A2 == 1
#include "mpl3115a2.h"
#include "mpl3115a2_params.h"
#endif


#if MODULE_BME680 == 1
#include "bme680.h"
#include "bme680_params.h"
#endif

#if GPS == 1
#include "gps.h"
#endif

#include "app_clock.h"
#include "benchmark.h"

#include <random.h>

/* Declare globally the loramac descriptor */
extern semtech_loramac_t loramac;

/* Declare globally the sensor device descriptor */
#if MODULE_DS75LX == 1
ds75lx_t ds75lx;
#endif

#if MODULE_AT30TES75X == 1
at30tse75x_t at30tse75x;
#endif

#if MODULE_MAG3110 == 1
static mag3110_t mag3110;
#endif

#if MODULE_MMA8X5X == 1
static mma8x5x_t mma8x5x;
#endif

#if MODULE_MPL3115A2 == 1
static mpl3115a2_t mpl3115a2;
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
#define PORT_DN_REBOOT_NOW           	64
#define PORT_DN_REBOOT_ONE_MINUTE       65
#define PORT_DN_REBOOT_ONE_HOUR         66


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
    int result;

	(void) result;

#if GPS == 1
    DEBUG("[gps] GPS is enabled (baudrate=%d)\n",STD_BAUDRATE);
#endif

#if MODULE_DS75LX == 1
    DEBUG("[ds75lx] DS75LX sensor is enabled\n");

    result = ds75lx_init(&ds75lx, &ds75lx_params[0]);
    if (result != DS75LX_OK)
    {
        DEBUG("[error] Failed to initialize DS75LX sensor\n");
        port = PORT_UP_ERROR;
    }
#endif

#if MODULE_AT30TES75X == 1
    DEBUG("[at30tse75x] AT30TES75X sensor is enabled\n");

    result = at30tse75x_init(&at30tse75x, PORT_A, AT30TSE75X_TEMP_ADDR);
    if (result != 0)
    {
        DEBUG("[error] Failed to initialize AT30TES75X sensor\n");
        port = PORT_UP_ERROR;
    }
#endif

#if MODULE_MAG3110 == 1
    puts("MAG3110 magnetometer driver test application\n");
    printf("Initializing MAG3110 magnetometer at I2C_%i... ",
           mag3110_params[0].i2c);
    if (mag3110_init(&mag3110, &mag3110_params[0]) != MAG3110_OK) {
        DEBUG("[error] Failed to initialize MAG3110 sensor\n");
        port = PORT_UP_ERROR;
    }
#endif

#if MODULE_MMA8X5X == 1
    puts("MMA8652 accelerometer driver test application\n");
    printf("Initializing MMA8652 accelerometer at I2C_DEV(%i)... ", mma8x5x_params->i2c);

    result = mma8x5x_init(&mma8x5x, mma8x5x_params);
    if(result != MMA8X5X_OK) {
    	DEBUG("[error] Failed to initialize MMA8X5X sensor\n");
    	port = PORT_UP_ERROR;
    }
#endif

#if MODULE_MPL3115A2 == 1
	result = mpl3115a2_init(&mpl3115a2, &mpl3115a2_params[0]);
	if(result != MPL3115A2_OK) {
    	DEBUG("[error] Failed to initialize MMA8X5X sensor\n");
    	port = PORT_UP_ERROR;
	}

	if (mpl3115a2_set_active(&mpl3115a2) != MPL3115A2_OK) {
		puts("[FAILED] activate measurement!");
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

#if MODULE_DS75LX == 1
	{
    /* measure temperature */
    ds75lx_wakeup(&ds75lx);
    /* Get temperature in degrees celsius */
    ds75lx_read_temperature(&ds75lx, &temperature);
    ds75lx_shutdown(&ds75lx);
    DEBUG("[ds75lx] get temperature : temperature=%d\n",temperature);
	}
#endif

#if MODULE_AT30TES75X == 1
    {
    /* measure temperature */
    //at30tse75x_wakeup(&at30tse75x);
    /* Get temperature in degrees celsius */
    float ftemp;
    at30tse75x_get_temperature(&at30tse75x, &ftemp);
    temperature = (int16_t)(ftemp * 100);
    //at30tse75x_shutdown(&at30tse75x);
    DEBUG("[at30tse75x] get temperature : temperature=%d\n",temperature);
    }
#endif

    unsigned int i = 0;

    // Encode temperature.
	payload[i++] = (temperature >> 8) & 0xFF;
	payload[i++] = (temperature >> 0) & 0xFF;

	if(len < i + (3*4)) {
		return i;
	}

#if MODULE_MAG3110 == 1
    {
    mag3110_data_t data;
    int8_t temp;
    mag3110_read(&mag3110, &data);
    printf("Field strength: X: %d Y: %d Z: %d\n", data.x, data.y, data.z);
    mag3110_read_dtemp(&mag3110, &temp);
    printf("Die Temperature T: %d\n", temp);
    // TODO add to payload
    }
#endif

	if(len < i + (3*4)) {
		return i;
	}

#if MODULE_MMA8X5X == 1
    {
    mma8x5x_data_t data;
    mma8x5x_read(&mma8x5x, &data);

    printf("Acceleration [in mg]: X: %d Y: %d Z: %d\n", data.x, data.y, data.z);
    // TODO add to payload
    }
#endif

    if(len < i + (2+2+1)) {
		return i;
	}

#if MODULE_MPL3115A2 == 1
    {
    uint32_t pressure;
    int16_t temperature;
    uint8_t status;
    if ((mpl3115a2_read_pressure(&mpl3115a2, &pressure, &status) |
         mpl3115a2_read_temp(&mpl3115a2, &temperature)) != MPL3115A2_OK) {
        puts("[FAILED] read MPL3115A2 values!");
    } else {
        printf("Pressure: %u Pa, Temperature: %3d.%d C, State: %#02x\n",
               (unsigned int)pressure, temperature/10, abs(temperature%10), status);
    }

	payload[i++] = (pressure >> 24) & 0xFF;
	payload[i++] = (pressure >> 16) & 0xFF;

	payload[i++] = (temperature >> 8) & 0xFF;
	payload[i++] = (temperature >> 0) & 0xFF;

	payload[i++] = (status) & 0xFF;

    // TODO add to payload
    }
#endif

	if(len < i + (2*3)+ sizeof(int16_t)) {
		return i;
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

#if APP_CLOCK_SYNC == 1
	app_clock_send_app_time_req(&loramac);
    xtimer_sleep(tx_period);
#endif

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
                        	memcpy(&tx_period, loramac.rx_data.payload, sizeof(uint16_t));
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

                    case PORT_DN_REBOOT_NOW:
                        DEBUG("[dn] Reboot now. port: %d\n", loramac.rx_data.port);
            			pm_reboot();
                    	break;
                    case PORT_DN_REBOOT_ONE_MINUTE:
                        DEBUG("[dn] Reboot in 60 sec. port: %d\n", loramac.rx_data.port);
                        xtimer_sleep(60U);
            			pm_reboot();
                    	break;
                    case PORT_DN_REBOOT_ONE_HOUR:
                        DEBUG("[dn] Reboot in 3600 sec. port: %d\n", loramac.rx_data.port);
                        xtimer_sleep(3600U);
            			pm_reboot();
                    	break;

                    default:
                        DEBUG("[dn] Data received: ");
                        printf_ba(loramac.rx_data.payload, loramac.rx_data.payload_len);
                        DEBUG(", port: %d\n",loramac.rx_data.port);
                        break;
                }
                break;

			case SEMTECH_LORAMAC_RX_LINK_CHECK:
				DEBUG("[dn] Link check information:\n"
				   "  - Demodulation margin: %d\n"
				   "  - Number of gateways: %d\n",
				   loramac.link_chk.demod_margin,
				   loramac.link_chk.nb_gateways);
				break;

			case SEMTECH_LORAMAC_RX_CONFIRMED:
				DEBUG("[dn] Received ACK from network\n");
				break;

			case SEMTECH_LORAMAC_TX_SCHEDULE:
				DEBUG("[dn] The Network Server has pending data\n");
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

    // TODO add i2c_scanner for listing I2C devices

    app_clock_print_rtc();

    cpuid_info();
    loramac_info();

    /* initialize the sensors */
    init_sensors();

    /* initialize the loramac stack */
    //semtech_loramac_init(&loramac);

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

    //random_init_by_array(uint32_t init_key[], int key_length)
    random_init_by_array((void*)appkey, LORAMAC_APPKEY_LEN/sizeof(uint32_t));

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

    //random_init_by_array(uint32_t init_key[], int key_length)
    random_init_by_array((void*)appskey, LORAMAC_APPSKEY_LEN/sizeof(uint32_t));

#endif

#ifdef FCNT_UP
    semtech_loramac_set_uplink_counter(&loramac, FCNT_UP);
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
