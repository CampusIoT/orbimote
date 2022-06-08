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

#ifndef LORAMAC_UTILS_H
#define LORAMAC_UTILS_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

    char *loramac_utils_err_message(uint8_t errCode);

    uint8_t loramac_utils_join_retry_loop(semtech_loramac_t *loramac, uint8_t initDataRate, uint32_t nextRetryTime, uint32_t maxNextRetryTime);

    uint8_t loramac_utils_abp_join_retry_loop(semtech_loramac_t *loramac, uint8_t initDataRate, uint32_t nextRetryTime, uint32_t maxNextRetryTime);

    void loramac_utils_forge_euis_and_key(uint8_t *deveui, uint8_t *appeui, uint8_t *appkey, const uint8_t* secret);

    const char* loramac_utils_get_lorawan_network(const uint32_t devaddr);

    void printf_ba(const uint8_t* ba, size_t len);

#endif
