/* Bluetooth: Mesh data models
 *
 * Copyright (c) 2020 Sebastian Förster
 *
 */

#include <drivers/gpio.h>

#include "device_composition.h"

#include "../user_led.h"
#include "../board.h"
#include "../fwversion.h"

/* This value has special meaning depending on the context in which it used. 
 * Link Manager Protocol (LMP): This value may be used in the internal and interoperability 
 * tests before a Company ID has been assigned. This value shall not be used in shipping end products. 
 * Device ID Profile: This value is reserved as the default vendor ID when no Device ID service record 
 * is present in a remote device. */
#define BT_COMPANY_IDENTIFIER					0xFFFF //place holder

#define BT_MESH_MODEL_ID_GEN_SENSOR_SRV			0x2000
#define BT_MESH_MODEL_ID_GEN_STATUS_SRV			0x2001

#define BT_MESH_MODEL_ID_GEN_HEARTBEAT_CLI		0x2010

#define BT_MESH_MIC_SIZE						4 //byte
#define BT_MESH_OPCODE_VENDOR_SIZE				3 //byte

#define BT_MESH_USERMODEL_OP_GEN_SENSOR_GET		BT_MESH_MODEL_OP_3(0x11, BT_COMPANY_IDENTIFIER)
#define BT_MESH_USERMODEL_OP_GEN_SENSOR_STATUS	BT_MESH_MODEL_OP_3(0x12, BT_COMPANY_IDENTIFIER)

#define BT_MESH_USERMODEL_OP_GEN_STATUS_GET		BT_MESH_MODEL_OP_3(0x21, BT_COMPANY_IDENTIFIER)
#define BT_MESH_USERMODEL_OP_GEN_STATUS_STATUS	BT_MESH_MODEL_OP_3(0x22, BT_COMPANY_IDENTIFIER)

#define BT_MESH_USERMODEL_OP_GEN_HEARTBEAT_SET	BT_MESH_MODEL_OP_3(0x33, BT_COMPANY_IDENTIFIER)


/*
 * A total message size, including an opcode, is determined by 
 * the underlying transport layer, which may use a Segmentation and 
 * Reassembly (SAR) mechanism. To maximize performance and avoid 
 * the overhead of SAR, a design goal is to fit messages in a single segment. 
 * The transport layer provides up to 11 octets for a non-segmented message, 
 * leaving up to 10 octets that are available for parameters when using 
 * a 1-octet opcode, up to 9 octets available for parameters when using 
 * a 2-octet opcode, and up to 8 octets available for parameters when using 
 * a vendor-specific 3-octet opcode.
*/
//-> max 8 bytes
struct sensor_data_t {
	uint64_t 	sensor_states_bitmask:16;
	uint64_t	phase1:12;
	uint64_t	phase2:12;
	uint64_t	phase3:12;
	uint64_t	n:12;
} sensor_data;

struct status_data_t {
	uint64_t 	fw_ver1:4;
	uint64_t 	fw_ver2:4;
	uint64_t 	fw_ver3:4;
	uint64_t 	fw_ver4:4;
	uint64_t 	hw_ver:8;
	uint64_t	temperature:8;
} status_data;

struct heartbeat_data_t {
	uint64_t 	ttl:8;
} heartbeat_data;

int8_t onoff = -1;

static void attention_on(struct bt_mesh_model *model)
{
	user_led_set_state(USER_LED_STATE_ATTENTION, 0x00);
}

static void attention_off(struct bt_mesh_model *model)
{
	user_led_set_state(USER_LED_STATE_ALLOFF, 0x00);
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};
static struct bt_mesh_cfg_cli cfg_cli = {
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model_pub gen_onoff_pub;

static void heartbeat(uint8_t hops, uint16_t feat);

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,

#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_ENABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif

#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif

	.default_ttl = 7,

	/* 2 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),

	/* 3 transmissions with 20ms interval */
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),

	.hb_sub.func = heartbeat,
};



static void heartbeat(uint8_t hops, uint16_t feat)
{
	printk("Heartbeat hops: %u, feat: %u\n", hops, feat);
}

/////// Generic OnOff callbacks ////////////////
static uint8_t on_off_state = 0;
static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	uint8_t tid;

	onoff = net_buf_simple_pull_u8(buf);
	tid = net_buf_simple_pull_u8(buf);

	if (onoff == 0) {
		user_led_set_state(USER_LED_STATE_ALLOFF, 0x00);
		on_off_state = 0;
	}
	else if(onoff == 1)
	{
		user_led_set_state(USER_LED_STATE_ALLON, 0x00);
		on_off_state = 1;
	}
	
	printk("NodeID: 0x%X04 TTL: %d RSSI: %d\n", ctx->recv_dst, ctx->recv_ttl, ctx->recv_rssi);
}

static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + BT_MESH_MIC_SIZE);
	
	printk("addr 0x%04x onoff 0x%02x\n",
	       bt_mesh_model_elem(model)->addr, on_off_state);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
	net_buf_simple_add_u8(&msg, on_off_state);

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		printk("Unable to send On Off Status response\n");
	}
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	gen_onoff_set_unack(model, ctx, buf);
	gen_onoff_get(model, ctx, buf);
}

/////// Sensor callbacks ////////////////
static void gen_sensor_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_OPCODE_VENDOR_SIZE + sizeof(sensor_data) + BT_MESH_MIC_SIZE);
	
	printk("addr 0x%04x sensor request\n", bt_mesh_model_elem(model)->addr);
	bt_mesh_model_msg_init(&msg, BT_MESH_USERMODEL_OP_GEN_SENSOR_STATUS);
	net_buf_simple_add_mem(&msg, &sensor_data, sizeof(sensor_data));

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		printk("Unable to send Sensor Status response\n");
	}
}

static int gen_sensor_update(struct bt_mesh_model *mod)
{
	struct net_buf_simple *msg = mod->pub->msg;

	printk("Preparing to send sensor data (publish)\n");

	bt_mesh_model_msg_init(msg, BT_MESH_USERMODEL_OP_GEN_SENSOR_STATUS);
	net_buf_simple_add_mem(msg, &sensor_data, sizeof(sensor_data));

	return 0;
}

/////// Status callbacks ////////////////
static void gen_status_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, BT_MESH_OPCODE_VENDOR_SIZE + sizeof(status_data) + BT_MESH_MIC_SIZE);
	
	printk("addr 0x%04x status request\n", bt_mesh_model_elem(model)->addr);
	bt_mesh_model_msg_init(&msg, BT_MESH_USERMODEL_OP_GEN_STATUS_STATUS);
	net_buf_simple_add_mem(&msg, &status_data, sizeof(status_data));

	if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
		printk("Unable to send Status response\n");
	}
}

static int gen_status_update(struct bt_mesh_model *mod)
{
	struct net_buf_simple *msg = mod->pub->msg;

	printk("Preparing to send status data (publish)\n");

	bt_mesh_model_msg_init(msg, BT_MESH_USERMODEL_OP_GEN_STATUS_STATUS);
	net_buf_simple_add_mem(msg, &status_data, sizeof(status_data));

	return 0;
}

//vendor model - 1 byte op code 2 byte vendor id
// gen_sensor_update -> periodic publish possible
BT_MESH_MODEL_PUB_DEFINE(gen_sensor_srv_pub_root, gen_sensor_update, BT_MESH_OPCODE_VENDOR_SIZE + sizeof(sensor_data));
BT_MESH_MODEL_PUB_DEFINE(gen_status_srv_pub_root, gen_status_update, BT_MESH_OPCODE_VENDOR_SIZE + sizeof(status_data));

BT_MESH_MODEL_PUB_DEFINE(gen_heartbeat_cli_pub_root, NULL, BT_MESH_OPCODE_VENDOR_SIZE + sizeof(heartbeat_data));

/////// heartbeat callbacks ////////////////
static void gen_heartbeat_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	int ttl = net_buf_simple_pull_u8(buf);
	
	int hops = ttl - ctx->recv_ttl + 1;

	//one hop is minimum... blink 5 times for direct connection to the sender (gateway)
	int led_blinks = 6 - hops;

	//blink one time minimum... 5 is max
	user_led_start_heartbeatblink( (led_blinks <= 0) ? 1 : led_blinks);

	printk("Heartbeat from 0x%04x over %d hop(s), last hop RSSI: %d\n", ctx->addr, hops, ctx->recv_rssi);
}


////// Define Decription Models ///////

// Generic OnOff
static const struct bt_mesh_model_op gen_onoff_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

// User (Walther) defined Sensor Model
static const struct bt_mesh_model_op gen_sensor_op[] = {
	{ BT_MESH_USERMODEL_OP_GEN_SENSOR_GET, 0, gen_sensor_get },
	BT_MESH_MODEL_OP_END,
};

// User (Walther) defined Status Model
static const struct bt_mesh_model_op gen_status_op[] = {
	{ BT_MESH_USERMODEL_OP_GEN_STATUS_GET, 0, gen_status_get },
	BT_MESH_MODEL_OP_END,
};

// User (Walther) defined Heartbeat Model
static const struct bt_mesh_model_op gen_heartbeat_op[] = {
	{ BT_MESH_USERMODEL_OP_GEN_HEARTBEAT_SET, 1, gen_heartbeat_set_unack },
	BT_MESH_MODEL_OP_END,
};

// add models to a root model
static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_op,
		      &gen_onoff_pub, NULL)
};

static struct bt_mesh_model vendor_models[] = {
	BT_MESH_MODEL_VND(BT_COMPANY_IDENTIFIER, BT_MESH_MODEL_ID_GEN_SENSOR_SRV, gen_sensor_op,
		      &gen_sensor_srv_pub_root, NULL),
	BT_MESH_MODEL_VND(BT_COMPANY_IDENTIFIER, BT_MESH_MODEL_ID_GEN_STATUS_SRV, gen_status_op,
		      &gen_status_srv_pub_root, NULL),
	BT_MESH_MODEL_VND(BT_COMPANY_IDENTIFIER, BT_MESH_MODEL_ID_GEN_HEARTBEAT_CLI, gen_heartbeat_op,
		      &gen_heartbeat_cli_pub_root, NULL)
};

//this node has only one element -> root model
static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vendor_models),
};

const struct bt_mesh_comp comp = {
	.cid = BT_COMPANY_IDENTIFIER,
	.pid = WALTHER_PRODUCT_ID,
	.vid = WALTHER_VERSION_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

void refresh_sensordata(uint16_t bitmask, uint16_t l1, uint16_t l2, uint16_t l3, uint16_t n)
{
	//save data for get and publish update command
	sensor_data.sensor_states_bitmask = bitmask;
	sensor_data.phase1 = l1;
	sensor_data.phase2 = l2;
	sensor_data.phase3 = l3;
	sensor_data.n = n;
}

void refresh_statusdata(int8_t temperature)
{
	//save data for get and publish update command
	status_data.temperature = temperature;
	status_data.fw_ver1 = CURRENT_FIMRWARE_VERSION1;
	status_data.fw_ver2 = CURRENT_FIMRWARE_VERSION2;
	status_data.fw_ver3 = CURRENT_FIMRWARE_VERSION3;
	status_data.fw_ver4 = CURRENT_FIMRWARE_VERSION4;
	status_data.hw_ver = 0;
}
