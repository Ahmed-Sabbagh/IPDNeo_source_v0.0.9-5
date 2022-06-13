/* filter.c - handle filter usage */

/*
 * Copyright (c) 2020 tecVenture
 *
 */

#include "filter.h"

/**
 * Put a value into the running average filter and get the filtered value as return
 *
 * @param filter filter instance as pointer
 * @param new_value new value to filter
 * @return filtered value
 */
float runavrfilter(RunAVRFilter_t *filter, float new_value)
{
	//store latest value for later usage
	filter->latest_value = new_value;
	
	if(!filter->first_time_used)
	{
		runavrfilter_restart(filter);
		filter->first_time_used = 1;
	}

	//filter time to get some average
	filter->filter_sum = filter->filter_sum - (filter->filter_sum  / (float)(1U << filter->bit_depth)) + new_value;

	return (filter->filter_sum / (float)(1U << filter->bit_depth));
}

/**
 * Faster settling of the filter. Takes the last value from shift filter and sets this as start value.
 *
 * @param filter filter instance as pointer
 */
void runavrfilter_restart(RunAVRFilter_t *filter)
{
	//filter time to get some average
	filter->filter_sum = filter->latest_value * (float)(1U << filter->bit_depth);
}
