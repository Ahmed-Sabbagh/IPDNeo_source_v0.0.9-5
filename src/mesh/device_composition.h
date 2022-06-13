/* Bluetooth: Mesh data models
 *
 * Copyright (c) 2020 Sebastian Förster
 */

#ifndef _DEVICE_COMPOSITION_H
#define _DEVICE_COMPOSITION_H

#include "ble_mesh.h"

extern int8_t 						onoff;

extern const struct bt_mesh_comp 	comp;

void refresh_sensordata(uint16_t bitmask, uint16_t l1, uint16_t l2, uint16_t l3, uint16_t n);
void refresh_statusdata(int8_t temperature);

#endif
