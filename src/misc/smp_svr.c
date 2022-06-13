/* Bluetooth: IPDNeo
 *
 * Copyright (c) 2020 tecVenture
 *
 */

#include <assert.h>
#include <bluetooth/conn.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <stats/stats.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#if defined(CONFIG_MCUMGR)
#include <mgmt/buf.h>
#include <mgmt/smp_bt.h>
#endif

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif

void smp_svr_init(void)
{
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
}
