/*
 * Copyright (C) 2020 INRIA
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
#include <stddef.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "inttypes.h"

#include "net/loramac.h"
#include "semtech_loramac.h"

#include <string.h>
#include "cpu_conf.h"
#include "periph/cpuid.h"

#include "hashes/sha1.h"

// TODO print_loramac(semtech_loramac_t *loramac)

char *semtech_loramac_err_message(uint8_t errCode)
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
 */
uint8_t loramac_join_retry_loop(semtech_loramac_t *loramac, uint8_t initDataRate, uint32_t nextRetryTime, uint32_t maxNextRetryTime)
{
    // TODO print DevEUI, AppEUI, AppKey

    DEBUG("Starting join procedure: dr=%d\n", initDataRate);

    semtech_loramac_set_dr(loramac, initDataRate);

    uint8_t joinRes;
    while ((joinRes = semtech_loramac_join(loramac, LORAMAC_JOIN_OTAA)) != SEMTECH_LORAMAC_JOIN_SUCCEEDED)
    {
        DEBUG("Join procedure failed: code=%d (%s)\n", joinRes, semtech_loramac_err_message(joinRes));

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
                nextRetryTime *= 2;
            }
            else
            {
                nextRetryTime = maxNextRetryTime;
            }
        }
        DEBUG("Retry join procedure in %ld sec. at dr=%d\n", nextRetryTime, initDataRate);

        /* sleep JOIN_NEXT_TENTATIVE secs */
        xtimer_sleep(nextRetryTime);
    }

    DEBUG("Join procedure succeeded\n");
    return joinRes;
}

static const uint8_t appeui_mask[LORAMAC_APPEUI_LEN/2] = { 0xff, 0xff, 0xff, 0xff };

void printf_ba(const uint8_t* ba, size_t len) {
    for (unsigned int i = 0; i < len; i++) {
        DEBUG("%02x", ba[i]);
    }
}

/**
 * Forge the DevEUI, AppEUI and the AppKey from the CPU ID of the MCU and a secret array of bytes
 */
void loramac_forge_deveui(uint8_t *deveui, uint8_t *appeui, uint8_t *appkey, const uint8_t* secret)
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
