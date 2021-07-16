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
 * @brief       LoRaWAN Benchmark.
 *
 * @author      Didier Donsez <didier.donsez@univ-grenoble-alpes.fr>
 *
 * @}
 */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include "semtech_loramac.h"


#ifndef NEXT_BENCHMARK_RANDOM
// random pause in seconds before the next benchmark
#define NEXT_BENCHMARK_RANDOM   10
#endif


#ifndef APP_TIME_REQ_PERIOD
// for sending a APP_TIME_REQ every 100 messages
#define APP_TIME_REQ_PERIOD   1000
#endif

struct benchmark_t {
	uint32_t devaddr;
#ifdef DEVADDRS
	uint32_t* devaddr;
#endif
	uint8_t nb_virtual_devices;
	uint8_t min_port;
	uint8_t max_port;
	uint16_t *tx_period;
	uint8_t drpwsz_sequence_nb;
	uint8_t *drpwsz_sequence;
	bool txconfirmed;
	bool adr;
};

/**
 * Start the benchmark.
 *
 * @param loramac the LoRaMac context
 */
extern void benchmark_start(semtech_loramac_t *loramac, struct benchmark_t benchmark, unsigned int (*encode_sensors)(uint8_t*, const unsigned int));
#endif /* BENCHMARK_H */
