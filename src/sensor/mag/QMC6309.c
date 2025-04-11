#include <math.h>

#include <zephyr/logging/log.h>

#include "QMC6309.h"
#include "sensor/sensor_none.h"

// https://www.lcsc.com/datasheet/lcsc_datasheet_2410121623_QST-QMC6309_C5439871.pdf

#define QMC6309_OUTX_L_REG 0x01

// Status register {DRDY:1;OVFL:1;...}
#define QMC6309_STAT_REG 0x09

#define STAT_DATA_RDY_MASK 0b01
#define STAT_OVERFLOW_MASK 0b10

// Control register 1 {MD:2;:1;OSR:2;LPF:3;}
#define QMC6309_CTRL_REG_1 0x0A

#define MD_SUSPEND 0b00
#define MD_NORMAL  0b01
#define MD_SINGLE  0b10
#define MD_CONTINUOUS 0b11
#define MD_MASK 0b11

#define OSR_OFF 0b11
#define OSR_2 0b10
#define OSR_4 0b01
#define OSR_8 0b00
#define OSR_MASK(osr) ((osr) << 3)

#define LPF_OFF 0b000
#define LPF_2 0b001
#define LPF_4 0b010
#define LPF_8 0b011
#define LPF_16 0b100
#define LPF_MASK(lpf) ((lpf) << 5)

// Control register 2 {SET:2;RGN:2;ODR:3;SRT:1;}
#define QMC6309_CTRL_REG_2 0x0B

#define SET_RESET_ON 0b00
#define SET_ONLY 0b01
#define SET_RESET_OFF 0b11

#define RNG_32G 0b00
#define RNG_16G 0b01
#define RNG_8G 0b10
#define RNG_MASK(rng) ((rng) << 2)

#define ODR_1Hz  0b000
#define ODR_10Hz  0b001
#define ODR_50Hz  0b010
#define ODR_100Hz 0b011
#define ODR_200Hz 0b100
#define ODR_MASK(odr) ((odr) << 4)

#define SOFT_RESET_MASK 0x80
#define SOFT_RESET_CLEAR 0x00

static const float sensitivity = 1000 / 4000.0f; // ~0.25 mgauss/LSB @ 8G range

static uint8_t last_state = 0xff;
static bool lastOvfl = false;
static bool skipSingleTrigger = false;

LOG_MODULE_REGISTER(QMC6309, LOG_LEVEL_INF);

int qmc_init(float time, float *actual_time)
{
	// Reset
	ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_2, SOFT_RESET_MASK);
	ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_2, SOFT_RESET_CLEAR);
	k_msleep(10);
	// Init
	last_state = 0xff; // init state
	lastOvfl = skipSingleTrigger = false;
	// LOG_DBG("INIT");
	int err = qmc_update_odr(time, actual_time);
	return (err < 0 ? err : 0);
}

void qmc_shutdown(void)
{
	int err = ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_1, MD_SUSPEND);
	// LOG_DBG("MD_SUSPEND");
	if (err)
		LOG_ERR("Communication error");
}

int qmc_update_odr(float time, float *actual_time)
{
	int ODR;
	uint8_t MODR;
	uint8_t MD;

	if (time <= 0) // off
	{
		MD = MD_SUSPEND;
		ODR = 0;
	}
	else if (time == INFINITY) // oneshot/single
	{
		MD = MD_SINGLE;
		ODR = 0;
	}
	else
	{
		MD = MD_CONTINUOUS;
		ODR = 1 / time;
	}

	if (MD == MD_SUSPEND)
	{
		MODR = ODR_1Hz;
		time = 0; // off
	}
	else if (ODR > 100)
	{
		MODR = ODR_200Hz;
		time = 1.f / 200;
	}
	else if (ODR > 50)
	{
		MODR = ODR_100Hz;
		time = 1.f / 100;
	}
	else if (ODR > 25)
	{
		MODR = ODR_50Hz;
		time = 1.f / 50;
	}
	else if (ODR > 5)
	{
		MODR = ODR_10Hz;
		time = 1.f / 10;
	}
	else if (ODR > 0)
	{
		MODR = ODR_1Hz;
		time = 1.f;
	}
	else // single
	{
		MODR = ODR_1Hz;
		time = INFINITY;
	}

	uint8_t STAT = ODR_MASK(MODR) | MD;
	if (last_state == STAT)
		return 1;
	last_state = STAT;

	if (MD == MD_SINGLE)
		skipSingleTrigger = true;

	// if (time <= 0)
	// 	LOG_DBG("MD_SUSPEND");
	// else if (time == INFINITY)
	// 	LOG_DBG("MD_SINGLE");
	// else
	// 	LOG_DBG("MD_CONTINUOUS %d", (int) (1 / time));

	int err = ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_2, ODR_MASK(MODR) | RNG_MASK(RNG_8G) | SET_RESET_ON);
	err |= ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_1, LPF_MASK(LPF_2) | OSR_MASK(OSR_8) | MD);
	if (err)
		LOG_ERR("Communication error");

	*actual_time = time;
	return err;
}

void qmc_mag_oneshot(void)
{
	if (skipSingleTrigger)
	{
		// Skip setting MD_SINGLE twice
		skipSingleTrigger = false;
		return;
	}
	// write MD_SINGLE again to trigger a measurement
	int err = ssi_reg_write_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_CTRL_REG_1, LPF_MASK(LPF_2) | OSR_MASK(OSR_8) | MD_SINGLE);
	// LOG_DBG("trigger MD_SINGLE");
	if (err)
		LOG_ERR("Communication error");
}

void qmc_mag_read(float m[3])
{
	int err = 0;
	uint8_t status = 0;
	uint16_t retry = 0;
	while ((status & STAT_DATA_RDY_MASK) == 0) // wait for data ready flag
	{
		err |= ssi_reg_read_byte(SENSOR_INTERFACE_DEV_MAG, QMC6309_STAT_REG, &status);
		if(++retry == 1024) {
			LOG_WRN("Data ready status timeout!");
			break;
		}
	}
	uint8_t rawData[6];
	err |= ssi_burst_read(SENSOR_INTERFACE_DEV_MAG, QMC6309_OUTX_L_REG, rawData, 6);
	if (err)
		LOG_ERR("Communication error");
	if (status & STAT_OVERFLOW_MASK) // check overflow flag
	{
		if (lastOvfl == 0)
		{
			LOG_INF("Magnetometer overflow");
			lastOvfl = 1;
		}
	}
	else
	{
		lastOvfl = 0;
	}
	qmc_mag_process(rawData, m);
}

void qmc_mag_process(uint8_t *raw_m, float m[3])
{
	for (int i = 0; i < 3; i++) // x, y, z
	{
		m[i] = ((uint16_t*)raw_m)[i];
		m[i] *= sensitivity; // result in mGauss
	}
	// static uint8_t skip_cnt = 0;
	// if (++skip_cnt == 1)
	// {
	// 	int16_t x = ((uint16_t*)raw_m)[0];
	// 	int16_t y = ((uint16_t*)raw_m)[1];
	// 	int16_t z = ((uint16_t*)raw_m)[2];
	// 	LOG_DBG("%d %d %d", x, y, z);
	// 	skip_cnt = 0;
	// }
}

const sensor_mag_t sensor_mag_qmc6309 = {
	*qmc_init,
	*qmc_shutdown,

	*qmc_update_odr,

	*qmc_mag_oneshot,
	*qmc_mag_read,
	*mag_none_temp_read,

	*qmc_mag_process,
	QMC6309_OUTX_L_REG
};
