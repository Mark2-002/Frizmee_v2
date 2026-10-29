#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/printk.h>

static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_SERVICE_VAL);
static struct bt_uuid_128 custom_char_uuid = BT_UUID_INIT_128(BT_UUID_CUSTOM_CHAR_VAL);

static uint8_t data[80] = {0};
static bool notify_enabled = false;
static struct bt_conn *default_conn;
static bool ble_connected = false;
int start_time;
uint8_t *buf_ptr;
uint16_t recieved_payload_length;
bool request_recieved;
int8_t led_status = 1; /* 0-off 1- advertising 2-connected 3-custom 4-error */

#define BT_UUID_CUSTOM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9abc, 0xdef012345678)

#define BT_UUID_CUSTOM_CHAR_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x9abc, 0xdef012345679)

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("Notifications %s\n", notify_enabled ? "enabled" : "disabled");
}

static ssize_t read_data(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr,
						 void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, data, sizeof(data));
}

static uint8_t rx_buf[20];
static ssize_t write_my_char(struct bt_conn *conn,
							 const struct bt_gatt_attr *attr,
							 const void *buf, uint16_t len,
							 uint16_t offset, uint8_t flags)
{
	request_recieved = true;
	recieved_payload_length = len;
	printk("Printing from callback: ");
	buf_ptr = (uint8_t *)buf;
	for (int i = 0; i < recieved_payload_length; i++)
	{
		printk("%02X", buf_ptr[i]);
	}
	printk("\n");

	uint16_t copy_len = MIN(len, sizeof(rx_buf) - 1);
	memcpy(rx_buf, buf, copy_len);
	rx_buf[copy_len] = '\0';

	if (len == 0)
	{
		return 0;
	}
	return len;
}

BT_GATT_SERVICE_DEFINE(custom_svc,
					   BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),
					   BT_GATT_CHARACTERISTIC(&custom_char_uuid.uuid,
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
											  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
											  read_data, write_my_char, data),
					   BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)
{
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M)
	{
		printk("PHY updated. New PHY: 1M\n");
	}
	else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M)
	{
		printk("PHY updated. New PHY: 2M\n");
	}
	else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8)
	{
		printk("PHY updated. New PHY: Long Range\n");
	}
}
static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_phy_updated = on_le_phy_updated,
};
int notify_data(void)
{
	int err;
	int attempt = 0;
	if (!default_conn)
		return -ENOTCONN;
	if (!notify_enabled)
		return -EACCES;

	size_t data_len = 0; 
	for (size_t i = 0; i < sizeof(data); i++)
	{
		if (data[i] != 0)
			data_len = i + 1;
	}

	struct bt_gatt_notify_params params = {
		.attr = &custom_svc.attrs[2],
		.data = data,
		.len = data_len,
	};
	err = bt_gatt_notify(default_conn, params.attr, params.data, params.len);
	return -ENOMEM;
}

static void update_phy(struct bt_conn *conn)
{
	int err;
	const struct bt_conn_le_phy_param preferred_phy = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};

	err = bt_conn_le_phy_update(conn, &preferred_phy);
	if (err)
	{
		printk("bt_conn_le_phy_update() returned %d", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to: %s\n", addr);

	default_conn = bt_conn_ref(conn);
	ble_connected = true;
	led_status = 2;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from: %s (reason 0x%02x)\n", addr, reason);

	ble_connected = false;
	led_status = 4;
	k_msleep(1000);
	if (default_conn)
	{
		led_status = 1;
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

int8_t ble_init(void)
{
	int err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}
	printk("Bluetooth initialized\n");
	bt_conn_cb_register(&conn_callbacks);
	return 0;
}

int8_t ble_deinit(void)
{
	int err;
	if (default_conn)
	{
		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err)
		{
			printk("Disconnection failed (err %d)\n", err);
			return err;
		}
		bt_conn_unref(default_conn);
		default_conn = NULL;
		printk("Disconnection initiated\n");
	}
	err = bt_disable();
	if (err)
	{
		printk("Bluetooth disable failed (err %d)\n", err);
		return err;
	}
	printk("Bluetooth disabled\n");
	led_status = 0;
	return 0;
}

int8_t start_advertising(void)
{
	int err;

	struct bt_le_adv_param *adv_params =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
						BT_GAP_ADV_FAST_INT_MIN_2,
						BT_GAP_ADV_FAST_INT_MAX_2,
						NULL);

	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
	};
	err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}
	printk("Advertising successfully started\n");
	led_status = 1;
	return 0;
}

int8_t stop_advertising(void)
{
	int err = bt_le_adv_stop();
	if (err)
	{
		printk("Failed to stop advertising (err %d)\n", err);
		return err;
	}
	printk("Advertising successfully stopped\n");
	led_status = 0;
	return 0;
}

int8_t disconnect(void)
{
	int err;
	if (default_conn)
	{
		err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err)
		{
			printk("Disconnection failed (err %d)\n", err);
			return err;
		}
		bt_conn_unref(default_conn);
		default_conn = NULL;
		printk("Disconnection initiated\n");
	}
	else
	{
		printk("No active connection to disconnect\n");
	}
	led_status = 0;
	return 0;
}