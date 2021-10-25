/*
 * app_clock.c
 *
 * Implementation of LoRaWAN Application Layer Clock Synchronization v1.0.0 Specification
 * https://lora-alliance.org/resource-hub/lorawanr-application-layer-clock-synchronization-specification-v100
 *
 * LoRaWAN® Application Layer Clock Synchronization Specification, authored by the FUOTA Working Group of the
 * LoRa Alliance® Technical Committee, proposes an application layer messaging package running over LoRaWAN®
 * to synchronize the real-time clock of an end-device to the network’s GPS clock with second accuracy.
 *
 * This package is useful for end-devices which do not have access to other accurate time source.
 * An end-device using LoRaWAN 1.1 or above SHOULD use DeviceTimeReq MAC command instead of this package.
 * ClassB end-devices have a more efficient way of synchronizing their clock, the classB network beacon. They
 * SHOULD NOT use this package and directly use the beacon time information.
 * End-devices with an accurate external clock source (e.g.: GPS) SHOULD use that clock source instead.
 *
 * Remark: Since GPS clock sources can be jammed or spoofed, this package can be used for secure time distribution.
 *         https://wiki.eclipse.org/images/3/3a/Eclipse-IoTDay2020Grenoble-friedt.pdf
 */

/**
 * @ingroup     pkg_lorawan_app_clock
 * @{
 *
 * @file
 * @brief       Implementation of Implementation of LoRaWAN Application Layer Clock Synchronization v1.0.0 Specification.
 *
 * @author      Didier Donsez <didier.donsez@univ-grenoble-alpes.fr>
 *
 * @}
 */

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "app_clock.h"

#include "xtimer.h"
#include <time.h>

#include "net/loramac.h"
#include "semtech_loramac.h"
#include "loramac_utils.h"

#include "periph_conf.h"

#if MODULE_PERIPH_RTC == 1
#include "periph/rtc.h"
#endif


#define DEFAULT_TM {0,0,0,1,0,121,0,0,0}

// 1972 and 1976 have 366 days (DELTA_EPOCH_GPS is 315964800 seconds)
// GPS Epoch consists of a count of weeks and seconds of the week since 0 hours (midnight) Sunday 6 January 1980
#define DELTA_EPOCH_GPS ((365*8 + 366*2 + 5)*(24*60*60))

// The end-device responds by sending up to NbTransmissions AppTimeReq messages
// with the AnsRequired bit set to 0.
// The end-device stops re-transmissions of the AppTimeReq if a valid AppTimeAns is received.
// If the NbTransmissions field is 0, the command SHALL be silently discarded.
// The delay between consecutive transmissions of the AppTimeReq is application specific.
// TODO static unsigned int NbTransmissions = 0;

// TokenReq is a 4 bits counter initially set to 0. TokenReq is incremented (modulo 16) each time the end-device receives and processes successfully an AppTimeAns message.
static unsigned int TokenReq = 0;

// If the AnsRequired bit is set to 1 the end-device expects an answer whether its clock is well
// synchronized or not. If this bit is set to 0, this signals to the AS that it only needs to answer if
// the end-device clock is de-synchronized.
// TODO static unsigned int AnsRequired = 1;

// Period encodes the periodicity of the AppTimeReq transmissions. The actual periodicity in
// seconds is 128.2𝑃𝑒𝑟𝑖𝑜𝑑 ±𝑟𝑎𝑛𝑑(30) where 𝑟𝑎𝑛𝑑(30) is a random integer in the +/-30sec
// range varying with each transmission.
static bool isPeriodDefined = false;
static unsigned int Period = 0;

#define sent_buffer_SIZE ((1 + sizeof(APP_CLOCK_PackageVersionAns_t)) + (1 + sizeof(APP_CLOCK_DeviceAppTimePeriodicityAns_t)) + (1 + sizeof(APP_CLOCK_AppTimeReq_t)))

static uint8_t sent_buffer[sent_buffer_SIZE];

static uint32_t sent_buffer_cursor = 0;

static uint32_t sent_buffer_device_time_pos = 0;

static time_t lastTimeCorrection = 0; // 01/01/1970

/*
 * print a tm struct
 */
#define TM_YEAR_OFFSET      (1900)
/**
 * Print the time
 *
 * @param label the label prefixing the time
 * @param time the time
 */
static void print_time(const char *label, const struct tm *time) {
	DEBUG("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
			time->tm_year + TM_YEAR_OFFSET, time->tm_mon + 1, time->tm_mday,
			time->tm_hour, time->tm_min, time->tm_sec);
}

/**
 * Print the RTC time
 */
void app_clock_print_rtc(void) {
	/* read RTC */
	struct tm current_time = DEFAULT_TM;
#if MODULE_PERIPH_RTC == 1
	rtc_get_time(&current_time);
#endif
	print_time("[clock] Current RTC time : ", &current_time);
	struct tm lastTimeCorrectionTime = *localtime(&lastTimeCorrection);
	if (lastTimeCorrection == 0) {
		DEBUG("[clock] Last correction  : never\n");
	} else {
		print_time("[clock] Last correction  : ", &lastTimeCorrectionTime);
	}
}

/**
 * Get the RTC time in seconds since 1/1/1980 (GPS time)
 */
static unsigned int getTimeSinceEpoch(void) {
	struct tm current_time = DEFAULT_TM;
	// Read the RTC current time
#if MODULE_PERIPH_RTC == 1
	rtc_get_time(&current_time);
#endif
	print_time("[clock] Current time: ", &current_time);
	time_t timeSinceEpoch = mktime(&current_time);
	// substract number of seconds between 6/1/1980 and 1/1/1970
	timeSinceEpoch -= DELTA_EPOCH_GPS;
	return timeSinceEpoch;
}

/**
 * Correct the RTC time
 *
 * @param timeCorrection the correction to apply to the RTC
 */
static void correct_rtc(int timeCorrection) {
	struct tm current_time = DEFAULT_TM;
	// Read the RTC current time
#if MODULE_PERIPH_RTC == 1
	rtc_get_time(&current_time);
#endif
	print_time("[clock] Current time    : ", &current_time);
	time_t timeSinceEpoch = mktime(&current_time);
	// Apply correction
	timeSinceEpoch += timeCorrection;
	DEBUG("[clock] Time Correction : %d\n", timeCorrection);
	current_time = *localtime(&timeSinceEpoch);
#if MODULE_PERIPH_RTC == 1
	rtc_set_time(&current_time);
#endif
	lastTimeCorrection = mktime(&current_time);
	print_time("[clock] RTC time fixed  : ", &current_time);
}

/**
 * Set the RTC time
 *
 * @param timeToSet the time in seconds since 6/1/1980 (GPS start time)
 */
static void set_rtc(unsigned int timeToSet) {
	struct tm current_time = DEFAULT_TM;
	// Read the RTC current time
#if MODULE_PERIPH_RTC == 1
	rtc_get_time(&current_time);
#endif
	print_time("[clock] Current time    : ", &current_time);
	time_t _TimeToSet = timeToSet + DELTA_EPOCH_GPS;
	current_time = *localtime(&_TimeToSet);
#if MODULE_PERIPH_RTC == 1
	rtc_set_time(&current_time);
#endif
	lastTimeCorrection = mktime(&current_time);
	print_time("[clock] RTC time fixed  : ", &current_time);
}

int8_t app_clock_process_downlink(semtech_loramac_t *loramac) {
	DEBUG("[clock] app_clock_process_downlink\n");

	uint32_t len = loramac->rx_data.payload_len;

	uint32_t idx = 0;

	uint8_t *payload = (uint8_t*) loramac->rx_data.payload;

	int8_t error = APP_CLOCK_OK;

	sent_buffer_cursor = 0;

	bool contains_APP_CLOCK_CID_PackageVersionReq = false;
	bool contains_APP_CLOCK_CID_DeviceAppTimePeriodicityReq = false;
	bool contains_APP_CLOCK_CID_AppTimeAns = false;
	bool contains_APP_CLOCK_CID_ForceDeviceResyncReq = false;
#ifdef EXPERIMENTAL
	bool contains_X_APP_CLOCK_CID_AppTimeSetReq = false;
#endif

	while (idx < len && (error == APP_CLOCK_OK )) {
		uint8_t cid = payload[idx];
		switch (cid) {
		case APP_CLOCK_CID_PackageVersionReq :
			DEBUG("[clock] APP_CLOCK_CID_PackageVersionReq\n")
			;
			if (contains_APP_CLOCK_CID_PackageVersionReq) {
				error = APP_CLOCK_CID_ALREADY_PROCESS;
				DEBUG("[clock] APP_CLOCK_CID_PackageVersionReq, error=%d\n", error);
				break;
			}
			contains_APP_CLOCK_CID_PackageVersionReq = true;
			if (idx + 1 + 0 <= len) {

				sent_buffer[sent_buffer_cursor] =
						APP_CLOCK_CID_PackageVersionAns;
				APP_CLOCK_PackageVersionAns_t *pva =
						(APP_CLOCK_PackageVersionAns_t*) (sent_buffer
								+ (1 + sent_buffer_cursor));
				pva->PackageIdentifier = 1;
				pva->PackageVersion = 1;

				sent_buffer_cursor +=
						(1 + sizeof(APP_CLOCK_PackageVersionAns_t));
				idx += 1;
			} else {
				error = APP_CLOCK_ERROR_OVERFLOW;
				DEBUG("[clock] APP_CLOCK_CID_PackageVersionReq, error=%d\n", error);
			}
			break;

		case APP_CLOCK_CID_DeviceAppTimePeriodicityReq :
			DEBUG("[clock] APP_CLOCK_CID_DeviceAppTimePeriodicityReq\n")
			;
			if (contains_APP_CLOCK_CID_DeviceAppTimePeriodicityReq) {
				error = APP_CLOCK_CID_ALREADY_PROCESS;
				DEBUG("[clock] APP_CLOCK_CID_DeviceAppTimePeriodicityReq, error=%d\n",
						error);
				break;
			}
			contains_APP_CLOCK_CID_DeviceAppTimePeriodicityReq = true;

			if (idx + 1 + sizeof(APP_CLOCK_DeviceAppTimePeriodicityReq_t)
					<= len) {
				APP_CLOCK_DeviceAppTimePeriodicityReq_t *datpr =
						(APP_CLOCK_DeviceAppTimePeriodicityReq_t*) (payload
								+ (idx + 1));

				isPeriodDefined = true;
				Period = datpr->Period;

				sent_buffer[sent_buffer_cursor] =
						APP_CLOCK_CID_DeviceAppTimePeriodicityAns;
				APP_CLOCK_DeviceAppTimePeriodicityAns_t *datpa =
						(APP_CLOCK_DeviceAppTimePeriodicityAns_t*) (sent_buffer
								+ (1 + sent_buffer_cursor));
				sent_buffer_device_time_pos = 1 + sent_buffer_cursor;
				datpa->NotSupported = 0; // The endpoint is not supporting periodicity currently
				datpa->Time = getTimeSinceEpoch();

				sent_buffer_cursor += (1
						+ sizeof(APP_CLOCK_DeviceAppTimePeriodicityAns_t));
				idx += (1 + sizeof(APP_CLOCK_DeviceAppTimePeriodicityReq_t));
			} else {
				error = APP_CLOCK_ERROR_OVERFLOW;
				DEBUG("[clock] APP_CLOCK_CID_DeviceAppTimePeriodicityReq, error=%d\n",
						error);
			}
			break;

		case APP_CLOCK_CID_AppTimeAns :
			DEBUG("[clock] APP_CLOCK_CID_AppTimeAns\n")
			;
			if (contains_APP_CLOCK_CID_AppTimeAns) {
				error = APP_CLOCK_CID_ALREADY_PROCESS;
				DEBUG("[clock] APP_CLOCK_CID_AppTimeAns, error=%d\n", error);
				break;
			}
			contains_APP_CLOCK_CID_AppTimeAns = true;

			if (idx + 1 + sizeof(APP_CLOCK_AppTimeAns_t) <= len) {
				APP_CLOCK_AppTimeAns_t *ata = (APP_CLOCK_AppTimeAns_t*) (payload
						+ (idx + 1));

				unsigned int TokenAns = ata->TokenAns;
				if (TokenAns != TokenReq) {
					error = APP_CLOCK_BAD_TOKEN;
					DEBUG("[clock] APP_CLOCK_CID_AppTimeAns, error=%d\n", error);
					break;
				}

				correct_rtc(ata->TimeCorrection);

				// increment TokenReq
				TokenReq++;
				TokenReq %= 16;

				idx += (1 + sizeof(APP_CLOCK_AppTimeAns_t));
			} else {
				error = APP_CLOCK_ERROR_OVERFLOW;
				DEBUG("[clock] APP_CLOCK_CID_AppTimeAns, error=%d\n", error);
			}
			break;

		case APP_CLOCK_CID_ForceDeviceResyncReq :
			DEBUG("[clock] APP_CLOCK_CID_ForceDeviceResyncReq\n")
			;
			if (contains_APP_CLOCK_CID_ForceDeviceResyncReq) {
				error = APP_CLOCK_CID_ALREADY_PROCESS;
				DEBUG("[clock] APP_CLOCK_CID_ForceDeviceResyncReq, error=%d\n", error);
				break;
			}
			contains_APP_CLOCK_CID_ForceDeviceResyncReq = true;

			if (idx + 1 + sizeof(APP_CLOCK_ForceDeviceResyncReq_t) <= len) {
				APP_CLOCK_ForceDeviceResyncReq_t *fdrr =
						(APP_CLOCK_ForceDeviceResyncReq_t*) (payload + (idx + 1));
				unsigned int NbTransmissions = fdrr->NbTransmissions;
				(void) NbTransmissions;
				// TODO

				idx += (1 + sizeof(APP_CLOCK_ForceDeviceResyncReq_t));
				error = APP_CLOCK_NOT_IMPLEMENTED;
				DEBUG("[clock] APP_CLOCK_CID_ForceDeviceResyncReq, error=%d\n", error);
			} else {
				error = APP_CLOCK_ERROR_OVERFLOW;
				DEBUG("[clock] APP_CLOCK_CID_ForceDeviceResyncReq, error=%d\n", error);
			}
			break;

#ifdef EXPERIMENTAL
		case X_APP_CLOCK_CID_AppTimeSetReq :
			DEBUG("[clock] X_APP_CLOCK_CID_AppTimeSetReq\n")
			;
			if (contains_X_APP_CLOCK_CID_AppTimeSetReq) {
				error = APP_CLOCK_CID_ALREADY_PROCESS;
				DEBUG("[clock] X_APP_CLOCK_CID_AppTimeSetReq, error=%d\n", error);
				break;
			}
			contains_X_APP_CLOCK_CID_AppTimeSetReq = true;

			if (idx + 1 + sizeof(X_APP_CLOCK_AppTimeSetReq_t) <= len) {
				X_APP_CLOCK_AppTimeSetReq_t *atsr =
						(X_APP_CLOCK_AppTimeSetReq_t*) (payload + (idx + 1));
				set_rtc(atsr->TimeToSet);
				idx += (1 + sizeof(X_APP_CLOCK_AppTimeSetReq_t));
			} else {
				error = APP_CLOCK_ERROR_OVERFLOW;
				DEBUG("[clock] X_APP_CLOCK_CID_AppTimeSetReq, error=%d\n", error);
			}
			break;
#endif

		default:
			error = APP_CLOCK_UNKNOWN_CID;
			DEBUG("[clock] APP_CLOCK : Unknown CID, error=%d\n", error)
			;
			break;
		}
	}

	DEBUG("[clock] sent_buffer:");
	printf_ba(sent_buffer, sent_buffer_cursor);
	DEBUG("\n");

	if (error == APP_CLOCK_OK) {
		error = app_clock_send_buffer(loramac);
	} else {
		sent_buffer_cursor = 0;
	}

	// TODO if NbTransmissions > 0, send an APP_CLOCK_CID_AppTimeReq

	return error;
}

int8_t app_clock_send_app_time_req(semtech_loramac_t *loramac) {
	DEBUG("[clock] app_clock_send_app_time_req\n");

	uint8_t payload[1 + sizeof(APP_CLOCK_AppTimeReq_t)];
	payload[0] = APP_CLOCK_CID_AppTimeReq;

	APP_CLOCK_AppTimeReq_t *atr = (APP_CLOCK_AppTimeReq_t*) (payload + 1);
	atr->TokenReq = TokenReq;
	atr->AnsRequired = 1;

	atr->DeviceTime = getTimeSinceEpoch();

	// save the current fPort and set the APP_CLOCK_PORT
	uint8_t current_fPort = semtech_loramac_get_tx_port(loramac);
	semtech_loramac_set_tx_port(loramac, APP_CLOCK_PORT);

	/* send the LoRaWAN message */
	uint8_t ret = semtech_loramac_send(loramac, payload,
			1 + sizeof(APP_CLOCK_AppTimeReq_t));

	int8_t error;
	if (ret != SEMTECH_LORAMAC_TX_DONE) {
		DEBUG("[clock] Cannot send buffer : ret code: %d (%s)\n", ret,
				loramac_utils_err_message(ret));
		if (ret == SEMTECH_LORAMAC_TX_SCHEDULE
				|| ret == SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED) {
			error = APP_CLOCK_TX_RETRY_LATER;
		} else {
			error = APP_CLOCK_TX_KO;
			// reset the buffer
			sent_buffer_cursor = 0;
		}
	} else {
		error = APP_CLOCK_OK;
	}

	// restore the current fPort
	semtech_loramac_set_tx_port(loramac, current_fPort);

	return error;
}

int8_t app_clock_send_buffer(semtech_loramac_t *loramac) {
	DEBUG("[clock] app_clock_send_buffer\n");

	int8_t error = APP_CLOCK_OK;

	if (sent_buffer_cursor != 0) {
		// save the current fPort and set the APP_CLOCK_PORT
		uint8_t current_fPort = semtech_loramac_get_tx_port(loramac);
		semtech_loramac_set_tx_port(loramac, APP_CLOCK_PORT);

		if (sent_buffer_device_time_pos != 0) {
			APP_CLOCK_DeviceAppTimePeriodicityAns_t *datpa =
					(APP_CLOCK_DeviceAppTimePeriodicityAns_t*) (sent_buffer
							+ (1 + sent_buffer_device_time_pos));
			datpa->Time = getTimeSinceEpoch();
		}
		/* send the LoRaWAN message */
		uint8_t ret = semtech_loramac_send(loramac, sent_buffer,
				sent_buffer_cursor);

		if (ret != SEMTECH_LORAMAC_TX_DONE) {
			DEBUG("[clock] Cannot send buffer : ret code: %d (%s)\n", ret,
					loramac_utils_err_message(ret));
			if (ret == SEMTECH_LORAMAC_TX_SCHEDULE
					|| ret == SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED) {
				error = APP_CLOCK_TX_RETRY_LATER;
			} else {
				error = APP_CLOCK_TX_KO;
				// reset the buffer
				sent_buffer_cursor = 0;
			}
		} else {
			// reset the buffer
			sent_buffer_cursor = 0;
		}

		// restore the current fPort
		semtech_loramac_set_tx_port(loramac, current_fPort);
	}

	return error;
}

bool app_clock_is_pending_buffer(void) {
	return sent_buffer_cursor != 0;
}
