/* filter.h - handle filter usage */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#ifndef _FILTER_H_
#define _FILTER_H_

#include <stdint.h>


typedef struct
{
	float		filter_sum; /**< stores the last values */
	uint8_t		bit_depth; /**< number of bits for filter */
	uint8_t		first_time_used;
	float		latest_value; /**< stores the latest pushed value and is used for faster filter settling with the reset function */
} RunAVRFilter_t;

/**
 * Put a value into the running average filter and get the filtered value as return
 *
 * @param filter filter instance as pointer
 * @param new_value new value to filter
 * @return filtered value
 */
float runavrfilter(RunAVRFilter_t *filter, float new_value);

/**
 * Faster settling of the filter. Takes the last value from shift filter and sets this as start value.
 *
 * @param filter filter instance as pointer
 */
void runavrfilter_restart(RunAVRFilter_t *filter);

#endif /* _FILTER_H_ */
