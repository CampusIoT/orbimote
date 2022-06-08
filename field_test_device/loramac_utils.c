/*
 * Copyright (C) 2020 Didier Donsez
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     pkg_semtech_loramac
 * @{
 *
 * @file
 * @brief       Utility functions for Semtech LoRaMac library.
 *
 * @author      Didier Donsez <didier.donsez@univ-grenoble-alpes.fr>
 *
 * @}
 */

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "inttypes.h"

#include "net/loramac.h"
#include "semtech_loramac.h"

#include <string.h>

#include "cpu_conf.h"
#include "periph/cpuid.h"

#ifdef FORGE_DEVEUI_APPEUI_APPKEY
#include "hashes/sha1.h"
#endif

#include "xtimer.h"

#include "loramac_utils.h"


#ifndef RETRYTIME_PERCENT
#define RETRYTIME_PERCENT (25U)
#endif

// TODO print_loramac(semtech_loramac_t *loramac)

void printf_ba(const uint8_t* ba, size_t len) {
	// TODO replace by fmt.h functions
    for (unsigned int i = 0; i < len; i++) {
        DEBUG("%02x", ba[i]);
    }
}

char *loramac_utils_err_message(uint8_t errCode)
{
    switch (errCode)
    {
    case SEMTECH_LORAMAC_JOIN_SUCCEEDED:
        return "Join procedure succeeded";
    case SEMTECH_LORAMAC_JOIN_FAILED:
        return "Join procedure failed";
    case SEMTECH_LORAMAC_NOT_JOINED:
        return "MAC is not joined";
    case SEMTECH_LORAMAC_ALREADY_JOINED:
        return "MAC is already joined";
    case SEMTECH_LORAMAC_TX_OK:
        return "Transmission is in progress";
    case SEMTECH_LORAMAC_TX_SCHEDULE:
        return "TX needs reschedule";
    case SEMTECH_LORAMAC_TX_DONE:
        return "Transmission completed";
    case SEMTECH_LORAMAC_TX_CNF_FAILED:
        return "Confirmable transmission failed";
    case SEMTECH_LORAMAC_TX_ERROR:
        return "Error in TX (invalid param, unknown service)";
    case SEMTECH_LORAMAC_RX_DATA:
        return "Data received";
    case SEMTECH_LORAMAC_RX_LINK_CHECK:
        return "Link check info received";
    case SEMTECH_LORAMAC_RX_CONFIRMED:
        return "Confirmed ACK received";
    case SEMTECH_LORAMAC_BUSY:
        return "Internal MAC is busy";
    case SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED:
        return "Restricted access to channels";
    default:
        return "Unknown reason";
    }
}

/**
 * start the OTAA join procedure (and retries if required)
 * @SEE https://lora-developers.semtech.com/documentation/tech-papers-and-guides/the-book/joining-and-rejoining
 */
uint8_t loramac_utils_join_retry_loop(semtech_loramac_t *loramac, uint8_t initDataRate, uint32_t nextRetryTime, uint32_t maxNextRetryTime)
{
    // TODO print DevEUI, AppEUI, AppKey

    DEBUG("[otaa] Starting join procedure: dr=%d\n", initDataRate);

    semtech_loramac_set_dr(loramac, initDataRate);

    uint8_t joinRes;
    while ((joinRes = semtech_loramac_join(loramac, LORAMAC_JOIN_OTAA)) != SEMTECH_LORAMAC_JOIN_SUCCEEDED)
    {
        DEBUG("[otaa] Join procedure failed: code=%d (%s)\n", joinRes, loramac_utils_err_message(joinRes));

        if (initDataRate > LORAMAC_JOIN_MIN_DATARATE)
        {
            /* decrement Join initDataRate */
            initDataRate--;
            semtech_loramac_set_dr(loramac, initDataRate);
        }
        else
        {
        	// TODO add random time to nextRetryTime

            /* double nextRetryTime in order to save the battery */
            if (nextRetryTime < maxNextRetryTime)
            {
                nextRetryTime += (nextRetryTime * RETRYTIME_PERCENT) / 100;
            }
            else
            {
                nextRetryTime = maxNextRetryTime;
            }
        }
        DEBUG("[otaa] Retry join procedure in %ld sec. at dr=%d\n", nextRetryTime, initDataRate);

        /* sleep JOIN_NEXT_TENTATIVE secs */
        xtimer_sleep(nextRetryTime);
    }

    DEBUG("[otaa] Join procedure succeeded\n");
    uint8_t devaddr[LORAMAC_DEVADDR_LEN];
    semtech_loramac_get_devaddr(loramac, devaddr);
	DEBUG("[otaa] DevAddr: "); printf_ba(devaddr,LORAMAC_DEVADDR_LEN); DEBUG("\n");

	// print nwkskey and appskey
	uint8_t key[LORAMAC_APPKEY_LEN];
	semtech_loramac_get_nwkskey(loramac,key);
	DEBUG("[otaa] NwkSKey:"); printf_ba(key,LORAMAC_APPKEY_LEN); DEBUG("\n");
	semtech_loramac_get_appskey(loramac,key);
	DEBUG("[otaa] AppSKey:"); printf_ba(key,LORAMAC_APPKEY_LEN); DEBUG("\n");
	uint32_t _devaddr = devaddr[3] << 24 & devaddr[2] << 16 & devaddr[1] << 8 & devaddr[0];
	DEBUG("[otaa] Network: %s\n",loramac_utils_get_lorawan_network(_devaddr));

	// TODO: print the Operator for the DevAddr with loramac_utils_get_lorawan_network(devaddr)

    return joinRes;
}

/**
 * start the OTAA join procedure (and retries if required)
 */
uint8_t loramac_utils_abp_join_retry_loop(semtech_loramac_t *loramac, uint8_t initDataRate, uint32_t nextRetryTime, uint32_t maxNextRetryTime)
{
    // TODO print DevEUI, AppEUI, AppKey

    DEBUG("[abp] Starting join procedure: dr=%d\n", initDataRate);

    semtech_loramac_set_dr(loramac, initDataRate);

    uint8_t joinRes;
    while ((joinRes = semtech_loramac_join(loramac, LORAMAC_JOIN_ABP)) != SEMTECH_LORAMAC_JOIN_SUCCEEDED)
    {
        DEBUG("[abp] Join procedure failed: code=%d (%s)\n", joinRes, loramac_utils_err_message(joinRes));

        if (initDataRate > 0)
        {
            /* decrement Join initDataRate */
            initDataRate--;
            semtech_loramac_set_dr(loramac, initDataRate);
        }
        else
        {
            /* double nextRetryTime in order to save the battery */
            if (nextRetryTime < maxNextRetryTime)
            {
                nextRetryTime += (nextRetryTime * RETRYTIME_PERCENT) / 100;
            }
            else
            {
                nextRetryTime = maxNextRetryTime;
            }
        }
        DEBUG("[abp] Retry join procedure in %ld sec. at dr=%d\n", nextRetryTime, initDataRate);

        /* sleep JOIN_NEXT_TENTATIVE secs */
        xtimer_sleep(nextRetryTime);
    }

    DEBUG("[abp] Join procedure succeeded\n");
    uint8_t devaddr[LORAMAC_DEVADDR_LEN];
    semtech_loramac_get_devaddr(loramac, devaddr);
	DEBUG("[abp] DevAddr:"); printf_ba(devaddr,LORAMAC_DEVADDR_LEN); DEBUG("\n");
	uint32_t _devaddr = devaddr[3] << 24 & devaddr[2] << 16 & devaddr[1] << 8 & devaddr[0];
	DEBUG("[abp] Network: %s\n",loramac_utils_get_lorawan_network(_devaddr));

	return joinRes;
}


#ifdef FORGE_DEVEUI_APPEUI_APPKEY

static const uint8_t appeui_mask[LORAMAC_APPEUI_LEN/2] = { 0xff, 0xff, 0xff, 0xff };


/**
 * Forge the DevEUI, AppEUI and the AppKey from the CPU ID of the MCU and a secret array of bytes
 */
void loramac_utils_forge_euis_and_key(uint8_t *deveui, uint8_t *appeui, uint8_t *appkey, const uint8_t* secret)
{
    uint8_t id[CPUID_LEN];
    /* read the CPUID */
    cpuid_get(id);

    if(CPUID_LEN > LORAMAC_DEVEUI_LEN) {
        memcpy(deveui,id+(CPUID_LEN-LORAMAC_DEVEUI_LEN),LORAMAC_DEVEUI_LEN);
    } else {
        memcpy(deveui+(LORAMAC_DEVEUI_LEN-CPUID_LEN),id,LORAMAC_DEVEUI_LEN);
    }
    memcpy(appeui,deveui,LORAMAC_APPEUI_LEN);
    memcpy(appeui+(LORAMAC_APPEUI_LEN/2),appeui_mask,LORAMAC_APPEUI_LEN/2);

    // Use secret for generating securely the appkey
    sha1_context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, deveui, LORAMAC_DEVEUI_LEN);
    sha1_update(&ctx, appeui, LORAMAC_APPEUI_LEN);
    sha1_update(&ctx, secret, LORAMAC_APPKEY_LEN);
    uint8_t digest[SHA1_DIGEST_LENGTH];
    sha1_final(&ctx, &digest);
    memcpy(appkey,digest,LORAMAC_APPKEY_LEN);
}
#endif


#define DEVADDR_MASK_NETID1								(0xFE000000)
#define DEVADDR_MASK_NETID3								(0xFFFE0000)
#define DEVADDR_MASK_NETID6								(0xFFFFFC00)


// Experimental NetID 1 (00000000 - 01FFFFFF)
#define DEVADDR_BASE_EXPERIMENTAL						(0x00000000)
// Experimental NetID 1 (02000000 - 03FFFFFF)
#define DEVADDR_BASE_EXPERIMENTAL1						(0x02000000)
// CampusIoT NetId 6 (FC00AC00 - FC00AFFF)
#define DEVADDR_BASE_UGA								(0xFC00AC00)
// TTN NetID 1 (26000000 - 27FFFFFF)
#define DEVADDR_BASE_TTN								(0x26000000)
// Actility NetID 1 (04000000 - 05FFFFFF)
#define DEVADDR_BASE_ACTILITY							(0x04000000)
// Orange NetID 1 (1E000000 - 1FFFFFFF)
#define DEVADDR_BASE_ORANGE								(0x1E000000)
// Bouygues Telecom NetID 1 (0E000000 - 0FFFFFFF)
#define DEVADDR_BASE_BOUYGUES_TELECOM					(0x0E000000)
// Requea NetId 6 (FC006800 - FC006BFF)
#define DEVADDR_BASE_REQUEA								(0xFC006800)


// Swisscom NetID 1 (08000000 - 09FFFFFF)
#define DEVADDR_BASE_SWISSCOM							(0x08000000)
// KPN NetID 1 (2A000000 - 2BFFFFFF)
#define DEVADDR_BASE_KPN								(0x2A000000)
// Digita NetID 3 (E0020000 - E003FFFF)
#define DEVADDR_BASE_DIGITA								(0xE0020000)

// Cisco Systems NetID 1 (2A000000 - 2BFFFFFF)
#define DEVADDR_BASE_CISCO_SYSTEMS						(0x2A000000)
// TATA Communication NetID 1(22000000 - 23FFFFFF)
#define DEVADDR_BASE_TATA_COMMUNICATIONS				(0x22000000)

// Lacuna NetId 6 (FC00A000 - FC00A3FF)
#define DEVADDR_BASE_LACUNA								(0xFC00A000)
// Hiber NetId 6 (FC008400 - FC0087FF)
#define DEVADDR_BASE_HIBER								(0xFC008400)

// Multitech NetID 1 (2E000000 - 2FFFFFFF)
#define DEVADDR_BASE_MULTITECH							(0x2E000000)
// Schneider Electric NetID 3 (E02E0000 - E02FFFFF)
#define DEVADDR_BASE_SCHNEIDER_ELECTRIC					(0xE02E0000)
// Kerlink NetID 1 (24000000 - 25FFFFFF)
#define DEVADDR_BASE_KERLINK							(0x24000000)


#define IS_BELONGING_TO_NETWORK(devaddr,devaddr_subnet,devaddr_mask) ( devaddr_subnet == ( devaddr & devaddr_mask ))


#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

typedef struct lorawan_network {

	/*
	 * @brief Subnet of the DevAddr
	 */
	uint32_t devaddr_subnet;

	/*
	 * @brief Mask of the DevAddr
	 */
	uint32_t devaddr_mask;

	/*
	 * @brief Name of the network
	 */
	char* name;

	// TODO add char* region;


} lorawan_network_t;

// TODO complete the list with https://www.thethingsnetwork.org/docs/lorawan/prefix-assignments/

static const lorawan_network_t lorawan_networks[] = {
//		{ DEVADDR_BASE_EXPERIMENTAL, DEVADDR_MASK_NETID1, "Experimental"},
//		{ DEVADDR_BASE_EXPERIMENTAL1, DEVADDR_MASK_NETID1, "Experimental1"},
		{ DEVADDR_BASE_ACTILITY, DEVADDR_MASK_NETID1, "Actility"},
		{ DEVADDR_BASE_TTN, DEVADDR_MASK_NETID1, "The Things Network"},
		{ DEVADDR_BASE_ORANGE, DEVADDR_MASK_NETID1, "Orange"},
		{ DEVADDR_BASE_BOUYGUES_TELECOM, DEVADDR_MASK_NETID1, "Bouygues Telecom"},
		{ DEVADDR_BASE_KERLINK, DEVADDR_MASK_NETID1, "Kerlink"},
		{ DEVADDR_BASE_CISCO_SYSTEMS, DEVADDR_MASK_NETID1, "Cisco Systems"},
		{ DEVADDR_BASE_TATA_COMMUNICATIONS, DEVADDR_MASK_NETID1, "Tata Communications"},
		{ DEVADDR_BASE_MULTITECH, DEVADDR_MASK_NETID1, "Mulitech"},

		{ DEVADDR_BASE_SCHNEIDER_ELECTRIC, DEVADDR_MASK_NETID3, "Schneider Electric"},

		{ DEVADDR_BASE_LACUNA, DEVADDR_MASK_NETID6, "Lacuna Space"},
		{ DEVADDR_BASE_HIBER, DEVADDR_MASK_NETID6, "Hiber"},
		{ DEVADDR_BASE_REQUEA, DEVADDR_MASK_NETID6, "Requea"},
		{ DEVADDR_BASE_UGA, DEVADDR_MASK_NETID6, "Université Grenoble Alpes"}
};

const char* loramac_utils_get_lorawan_network(const uint32_t devaddr) {
	// TODO Special case for Helium

	for(unsigned int i=0; i < NELEMS(lorawan_networks); i++) {
		const lorawan_network_t* ln = lorawan_networks + i;
		if (IS_BELONGING_TO_NETWORK(devaddr,ln->devaddr_subnet, ln->devaddr_mask)) {
			return ln->name;
		}
	}

	return "Unknown";
}

