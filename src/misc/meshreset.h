/* Bluetooth: IPDNeo
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#ifndef _MESHRESET_H
#define _MESHRESET_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/mesh.h>

#define MESHRESET_BOOTTIMEOUT_MS	7000UL

void meshreset_boothook(void);

#endif
