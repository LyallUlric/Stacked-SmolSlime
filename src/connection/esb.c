/*
	SlimeVR Code is placed under the MIT license
	Copyright (c) 2025 SlimeVR Contributors

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/
#include "globals.h"
#include "system/system.h"
#include "connection.h"

#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#if defined(NRF54L15_XXAA)
#include <hal/nrf_clock.h>
#endif /* defined(NRF54L15_XXAA) */
#include <zephyr/sys/crc.h>
#if defined(CONFIG_CLOCK_CONTROL_NRF2)
#include <hal/nrf_lrcconf.h>
#endif

#include "esb.h"

uint8_t last_reset = 0;
//const nrfx_timer_t m_timer = NRFX_TIMER_INSTANCE(1);
bool esb_state = false;
bool timer_state = false;
bool send_data = false;
uint16_t led_clock = 0;
uint32_t led_clock_offset = 0;

uint32_t tx_errors = 0;

static struct esb_payload rx_payload;
static struct esb_payload tx_payload = ESB_CREATE_PAYLOAD(0,
														  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
static struct esb_payload tx_payload_pair = ESB_CREATE_PAYLOAD(0,
														  0, 0, 0, 0, 0, 0, 0, 0);

static uint8_t paired_addr[8] = {0};

static bool esb_initialized = false;
static bool esb_paired = false;

LOG_MODULE_REGISTER(esb_event, LOG_LEVEL_INF);

void event_handler(struct esb_evt const *event)
{
	switch (event->evt_id)
	{
	case ESB_EVENT_TX_SUCCESS:
		if (tx_errors >= 100)
			set_status(SYS_STATUS_CONNECTION_ERROR, false);
		tx_errors = 0;
		if (esb_paired)
			clocks_stop();
		break;
	case ESB_EVENT_TX_FAILED:
		if (++tx_errors == 100) // consecutive failure to transmit
			set_status(SYS_STATUS_CONNECTION_ERROR, true);
		LOG_DBG("TX FAILED");
		if (esb_paired)
			clocks_stop();
		break;
	case ESB_EVENT_RX_RECEIVED:
		if (!esb_read_rx_payload(&rx_payload)) // zero, rx success
		{
			if (!paired_addr[0]) // zero, not paired
			{
				if (rx_payload.length == 8)
					memcpy(paired_addr, rx_payload.data, sizeof(paired_addr));
			}
			else
			{
				if (rx_payload.length == 4)
				{
					// TODO: Device should never receive packets if it is already paired, why is this packet received?
					// This may be part of acknowledge
//					if (!nrfx_timer_init_check(&m_timer))
					{
						LOG_WRN("Timer not initialized");
						break;
					}
					if (timer_state == false)
					{
//						nrfx_timer_resume(&m_timer);
						timer_state = true;
					}
//					nrfx_timer_clear(&m_timer);
					last_reset = 0;
					led_clock = (rx_payload.data[0] << 8) + rx_payload.data[1]; // sync led flashes :)
					led_clock_offset = 0;
					LOG_DBG("RX, timer reset");
				}
			}
		}
		break;
	}
}

bool clock_status = false;

#if defined(CONFIG_CLOCK_CONTROL_NRF)
int clocks_start(void)
{
	if (clock_status)
		return 0;
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;
	int fetch_attempts = 0;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr)
	{
		LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0)
	{
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do
	{
		k_usleep(100);
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res)
		{
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
		if (err && ++fetch_attempts > 10) {
			LOG_WRN("Unable to fetch Clock request result: %d", err);
			return err;
		}
	} while (err);

#if defined(NRF54L15_XXAA)
	/* MLTPAN-20 */
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
#endif /* defined(NRF54L15_XXAA) */

	LOG_DBG("HF clock started");
	clock_status = true;
	return 0;
}

void clocks_stop(void)
{
	clock_status = false;
	struct onoff_manager *clk_mgr;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	onoff_release(clk_mgr);

	LOG_DBG("HF clock stop request");
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF2)

int clocks_start(void)
{
	int err;
	int res;
	const struct device *radio_clk_dev =
		DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DT_NODELABEL(radio)));
	struct onoff_client radio_cli;

	/** Keep radio domain powered all the time to reduce latency. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_1, true);

	sys_notify_init_spinwait(&radio_cli.notify);

	err = nrf_clock_control_request(radio_clk_dev, NULL, &radio_cli);

	do {
		err = sys_notify_fetch_result(&radio_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err == -EAGAIN);

#if defined(NRF54L15_XXAA)
	/* MLTPAN-20 */
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
#endif /* defined(NRF54L15_XXAA) */

	LOG_DBG("HF clock started");
	return 0;
}

#else
BUILD_ASSERT(false, "No Clock Control driver");
#endif /* defined(CONFIG_CLOCK_CONTROL_NRF2) */

static struct k_thread clocks_thread_id;
static K_THREAD_STACK_DEFINE(clocks_thread_id_stack, 128);

void clocks_request_start(uint32_t delay_us)
{
	k_thread_create(&clocks_thread_id, clocks_thread_id_stack, K_THREAD_STACK_SIZEOF(clocks_thread_id_stack), (k_thread_entry_t)clocks_start, NULL, NULL, NULL, 5, 0, K_USEC(delay_us));
}

static struct k_thread clocks_stop_thread_id;
static K_THREAD_STACK_DEFINE(clocks_stop_thread_id_stack, 128);

void clocks_request_stop(uint32_t delay_us)
{
	k_thread_create(&clocks_stop_thread_id, clocks_stop_thread_id_stack, K_THREAD_STACK_SIZEOF(clocks_stop_thread_id), (k_thread_entry_t)clocks_stop, NULL, NULL, NULL, 5, 0, K_USEC(delay_us));
}

// this was randomly generated
// TODO: I have no idea?
static const uint8_t discovery_base_addr_0[4] = {0x62, 0x39, 0x8A, 0xF2};
static const uint8_t discovery_base_addr_1[4] = {0x28, 0xFF, 0x50, 0xB8};
static const uint8_t discovery_addr_prefix[8] = {0xFE, 0xFF, 0x29, 0x27, 0x09, 0x02, 0xB2, 0xD6};

static uint8_t base_addr_0[4], base_addr_1[4], addr_prefix[8] = {0};

int esb_initialize(bool tx)
{
	int err;

	struct esb_config config = ESB_DEFAULT_CONFIG;

	if (tx)
	{
		// config.protocol = ESB_PROTOCOL_ESB_DPL;
		// config.mode = ESB_MODE_PTX;
		config.event_handler = event_handler;
		// config.bitrate = ESB_BITRATE_2MBPS;
		// config.crc = ESB_CRC_16BIT;
		config.tx_output_power = CONFIG_RADIO_TX_POWER;
		// config.retransmit_delay = 600;
		//config.retransmit_count = 0;
		//config.tx_mode = ESB_TXMODE_MANUAL;
		// config.payload_length = 32;
		config.selective_auto_ack = true; // TODO: while pairing, should be set to false
		config.use_fast_ramp_up = true;
	}
	else
	{
		// config.protocol = ESB_PROTOCOL_ESB_DPL;
		config.mode = ESB_MODE_PRX;
		config.event_handler = event_handler;
		// config.bitrate = ESB_BITRATE_2MBPS;
		// config.crc = ESB_CRC_16BIT;
		config.tx_output_power = CONFIG_RADIO_TX_POWER;
		// config.retransmit_delay = 600;
		// config.retransmit_count = 3;
		// config.tx_mode = ESB_TXMODE_AUTO;
		// config.payload_length = 32;
		config.selective_auto_ack = true;
		config.use_fast_ramp_up = true;
	}

	err = esb_init(&config);

	if (!err)
		esb_set_base_address_0(base_addr_0);

	if (!err)
		esb_set_base_address_1(base_addr_1);

	if (!err)
		esb_set_prefixes(addr_prefix, ARRAY_SIZE(addr_prefix));

	if (err)
	{
		LOG_ERR("ESB initialization failed: %d", err);
		set_status(SYS_STATUS_CONNECTION_ERROR, true);
		return err;
	}

	esb_initialized = true;
	return 0;
}

void esb_deinitialize(void)
{
	if (esb_initialized)
		esb_disable();
	esb_initialized = false;
}

inline void esb_set_addr_discovery(void)
{
	memcpy(base_addr_0, discovery_base_addr_0, sizeof(base_addr_0));
	memcpy(base_addr_1, discovery_base_addr_1, sizeof(base_addr_1));
	memcpy(addr_prefix, discovery_addr_prefix, sizeof(addr_prefix));
}

inline void esb_set_addr_paired(void)
{
	// Recreate receiver address
	uint8_t addr_buffer[16] = {0};
	for (int i = 0; i < 4; i++)
	{
		addr_buffer[i] = paired_addr[i + 2];
		addr_buffer[i + 4] = paired_addr[i + 2] + paired_addr[6];
	}
	for (int i = 0; i < 8; i++)
		addr_buffer[i + 8] = paired_addr[7] + i;
	for (int i = 0; i < 16; i++)
	{
		if (addr_buffer[i] == 0x00 || addr_buffer[i] == 0x55 || addr_buffer[i] == 0xAA) // Avoid invalid addresses (see nrf datasheet)
			addr_buffer[i] += 8;
	}
	memcpy(base_addr_0, addr_buffer, sizeof(base_addr_0));
	memcpy(base_addr_1, addr_buffer + 4, sizeof(base_addr_1));
	memcpy(addr_prefix, addr_buffer + 8, sizeof(addr_prefix));
}

void esb_pair(void)
{
	// Read paired address from retained
	// TODO: should pairing data stay within esb?
	memcpy(paired_addr, retained->paired_addr, sizeof(paired_addr));

	if (!paired_addr[0]) // zero, no receiver paired
	{
		LOG_INF("Pairing");
		esb_set_addr_discovery();
		esb_initialize(true);
//		timer_init(); // TODO: shouldn't be here!!!
		tx_payload_pair.noack = false;
		uint64_t *addr = (uint64_t *)NRF_FICR->DEVICEADDR; // Use device address as unique identifier (although it is not actually guaranteed, see datasheet)
		memcpy(&tx_payload_pair.data[2], addr, 6);
		LOG_INF("Device address: %012llX", *addr & 0xFFFFFFFFFFFF);
		uint8_t checksum = crc8_ccitt(0x07, &tx_payload_pair.data[2], 6);
		if (checksum == 0)
			checksum = 8;
		LOG_INF("Checksum: %02X", checksum);
		tx_payload_pair.data[0] = checksum; // Use checksum to make sure packet is for this device
		set_led(SYS_LED_PATTERN_SHORT, SYS_LED_PRIORITY_CONNECTION);
		while (paired_addr[0] != checksum)
		{
			if (!clock_status)
				clocks_start();
			if (paired_addr[0])
			{
				LOG_INF("Incorrect checksum: %02X", paired_addr[0]);
				paired_addr[0] = 0; // Packet not for this device
			}
			esb_flush_rx();
			esb_flush_tx();
			esb_write_payload(&tx_payload_pair);
			esb_start_tx();
			k_msleep(1000);
		}
		set_led(SYS_LED_PATTERN_ONESHOT_COMPLETE, SYS_LED_PRIORITY_CONNECTION);
		LOG_INF("Paired");
		sys_write(PAIRED_ID, retained->paired_addr, paired_addr, sizeof(paired_addr)); // Write new address and tracker id
		esb_disable();
		k_msleep(1600); // wait for led pattern
	}
	LOG_INF("Tracker ID: %u", paired_addr[1]);
	LOG_INF("Receiver address: %012llX", (*(uint64_t *)&retained->paired_addr[0] >> 16) & 0xFFFFFFFFFFFF);

	connection_set_id(paired_addr[1]);

	esb_set_addr_paired();
	esb_paired = true;
	clocks_stop();
}

void esb_reset_pair(void)
{
	esb_deinitialize(); // make sure esb is off
	esb_paired = false;
	uint8_t empty_addr[8] = {0};
	sys_write(PAIRED_ID, &retained->paired_addr, empty_addr, sizeof(paired_addr)); // write zeroes
	LOG_INF("Pairing data reset");
}

void esb_write(uint8_t *data)
{
	if (!esb_initialized || !esb_paired)
		return;
	if (!clock_status)
		clocks_start(); 
#if defined(NRF54L15_XXAA) // TODO: esb halts with ack and tx fail
	tx_payload.noack = true;
#else
	tx_payload.noack = false;
#endif
	memcpy(tx_payload.data, data, tx_payload.length);
	esb_flush_tx(); // this will clear all transmissions even if they did not complete
	esb_write_payload(&tx_payload); // Add transmission to queue
	send_data = true;
}

bool esb_ready(void)
{
	return esb_initialized && esb_paired;
}
