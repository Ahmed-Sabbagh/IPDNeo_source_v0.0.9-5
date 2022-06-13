/* temp.h - handle internal chip temperature reading */

/*
 * Copyright (c) 2020 tecVenture
 *
 */
 
#ifndef TEMP_H_
#define TEMP_H_

#include <stdint.h>

void 	temp_init(void);
float 	temp_read(void);

#endif
