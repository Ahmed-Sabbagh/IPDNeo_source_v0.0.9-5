/* Bluetooth: Mesh Neo
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#ifndef _STORAGE_H
#define _STORAGE_H

#include <string.h>
#include <settings/settings.h>

enum workertask_e
{
	WORKERTASK_STORE_RESETCNT,
	WORKERTASK_EXE_MESHRESET,
};


int 	storage_init(void);
void 	storage_set_rc(uint8_t counter);
uint8_t 	storage_get_rc(void);
void 	storage_exe_task(enum workertask_e workertask_local);

#endif
