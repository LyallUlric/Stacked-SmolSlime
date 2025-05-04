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
#include "util.h"

#include <math.h>

#include "sensors_enum.h"
#include "magneto/magneto1_4.h"
#include "imu/BMI270.h"

#include "calibration.h"

static uint8_t imu_id;
static uint8_t sensor_data[128]; // any use sensor data

static float accelBias[3] = {0}, gyroBias[3] = {0}, magBias[3] = {0}; // offset biases

static float magBAinv[4][3];
static float accBAinv[4][3];

static uint8_t mag_progress;
static uint8_t last_mag_progress;
static int64_t mag_progress_time;

static double ata[100]; // init calibration
static double norm_sum;
static double sample_count;

LOG_MODULE_REGISTER(calibration, LOG_LEVEL_INF);

static void calibration_thread(void);
K_THREAD_DEFINE(calibration_thread_id, 1024, calibration_thread, NULL, NULL, NULL, 6, 0, 0);

static void sensor_sample_accel(const float a[3]);
static int sensor_wait_accel(float a[3], k_timeout_t timeout);

static void sensor_sample_gyro(const float g[3]);
static int sensor_wait_gyro(float g[3], k_timeout_t timeout);

static void sensor_sample_mag(const float m[3]);
static int sensor_wait_mag(float m[3], k_timeout_t timeout);

#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
static void magneto_reset(void);
#endif

void sensor_calibration_process_accel(float a[3])
{
	sensor_sample_accel(a);
#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
	apply_BAinv(a, accBAinv);
#else
	for (int i = 0; i < 3; i++)
		a[i] -= accelBias[i];
#endif
}

void sensor_calibration_process_gyro(float g[3])
{
	sensor_sample_gyro(g);
	for (int i = 0; i < 3; i++)
		g[i] -= gyroBias[i];
}

void sensor_calibration_process_mag(float m[3])
{
//	for (int i = 0; i < 3; i++)
//		m[i] -= magBias[i];
	sensor_sample_mag(m);
	apply_BAinv(m, magBAinv);
}

void sensor_calibration_update_sensor_ids(int imu)
{
	imu_id = imu;
}

uint8_t *sensor_calibration_get_sensor_data()
{
	return sensor_data;
}

int sensor_calibration_get_mag_progress()
{
    return mag_progress;
}

// TODO: isAccRest
bool wait_for_motion(bool motion, int samples)
{
	uint8_t counts = 0;
	float a[3], last_a[3];
	if (sensor_wait_accel(last_a, K_MSEC(1000)))
		return false;
	LOG_INF("Accelerometer: %.5f %.5f %.5f", (double)last_a[0], (double)last_a[1], (double)last_a[2]);
	for (int i = 0; i < samples + counts; i++)
	{
		k_msleep(500);
		if (sensor_wait_accel(a, K_MSEC(1000)))
			return false;
		LOG_INF("Accelerometer: %.5f %.5f %.5f", (double)a[0], (double)a[1], (double)a[2]);
		if (v_epsilon(a, last_a, 0.1) != motion)
		{
			LOG_INF("No motion detected");
			counts++;
			if (counts == 2)
				return true;
		}
		else
		{
			counts = 0;
		}
		memcpy(last_a, a, sizeof(a));
	}
	LOG_INF("Motion detected");
	return false;
}

void sensor_calibration_read(void)
{
	memcpy(sensor_data, retained->sensor_data, sizeof(sensor_data));
	memcpy(accelBias, retained->accelBias, sizeof(accelBias));
	memcpy(gyroBias, retained->gyroBias, sizeof(gyroBias));
	memcpy(magBias, retained->magBias, sizeof(magBias));
	memcpy(magBAinv, retained->magBAinv, sizeof(magBAinv));
	memcpy(accBAinv, retained->accBAinv, sizeof(accBAinv));
}

void sensor_calibrate_imu()
{
	float last_accelBias[3], last_gyroBias[3];
	memcpy(last_accelBias, accelBias, sizeof(accelBias));
	memcpy(last_gyroBias, gyroBias, sizeof(gyroBias));
	LOG_INF("Calibrating main accelerometer and gyroscope zero rate offset");
	LOG_INF("Rest the device on a stable surface");

	set_led(SYS_LED_PATTERN_LONG, SYS_LED_PRIORITY_SENSOR);
	if (!wait_for_motion(false, 6)) // Wait for accelerometer to settle, timeout 3s
	{
		set_led(SYS_LED_PATTERN_OFF, SYS_LED_PRIORITY_SENSOR);
		return; // Timeout, calibration failed
	}

	set_led(SYS_LED_PATTERN_ON, SYS_LED_PRIORITY_SENSOR);
	k_msleep(500); // Delay before beginning acquisition

	if (imu_id == IMU_BMI270) // bmi270 specific
	{
		LOG_INF("Suspending sensor thread");
		main_imu_suspend();
		LOG_INF("Running IMU specific calibration");
		int err = bmi_crt(sensor_data); // will automatically reinitialize // TODO: this blocks sensor!
		LOG_INF("Resuming sensor thread");
		main_imu_resume();
		if (err)
		{
			LOG_WRN("IMU specific calibration was not completed properly");
			set_led(SYS_LED_PATTERN_OFF, SYS_LED_PRIORITY_SENSOR);
			return; // Calibration failed
		}
		else
			sys_write(MAIN_SENSOR_DATA_ID, &retained->sensor_data, sensor_data, sizeof(sensor_data));
		k_msleep(500); // Delay before beginning acquisition
	}

	LOG_INF("Reading data");
	sensor_calibration_clear();
	int err = sensor_offsetBias(accelBias, gyroBias);
	if (err) // This takes about 3s
	{
		if (err == -1)
			LOG_INF("Motion detected");
		accelBias[0] = NAN; // invalidate calibration
	}
	else
	{
		sys_write(MAIN_ACCEL_BIAS_ID, &retained->accelBias, accelBias, sizeof(accelBias));
		sys_write(MAIN_GYRO_BIAS_ID, &retained->gyroBias, gyroBias, sizeof(gyroBias));
#if !CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
		LOG_INF("Accelerometer bias: %.5f %.5f %.5f", (double)accelBias[0], (double)accelBias[1], (double)accelBias[2]);
#endif
		LOG_INF("Gyroscope bias: %.5f %.5f %.5f", (double)gyroBias[0], (double)gyroBias[1], (double)gyroBias[2]);
	}
	if (sensor_calibration_validate())
	{
		set_led(SYS_LED_PATTERN_OFF, SYS_LED_PRIORITY_SENSOR);
		LOG_INF("Restoring previous calibration");
		memcpy(accelBias, last_accelBias, sizeof(accelBias)); // restore last calibration
		memcpy(gyroBias, last_gyroBias, sizeof(gyroBias)); // restore last calibration
#if !CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
		LOG_INF("Accelerometer bias: %.5f %.5f %.5f", (double)accelBias[0], (double)accelBias[1], (double)accelBias[2]);
#endif
		LOG_INF("Gyroscope bias: %.5f %.5f %.5f", (double)gyroBias[0], (double)gyroBias[1], (double)gyroBias[2]);
		sensor_calibration_validate(); // additionally verify old calibration
		return;
	}

	LOG_INF("Finished calibration");
	sensor_fusion_invalidate();
	set_led(SYS_LED_PATTERN_ONESHOT_COMPLETE, SYS_LED_PRIORITY_SENSOR);
}

#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
void sensor_calibrate_6_side(void)
{
	float last_accBAinv[4][3];
	memcpy(last_accBAinv, accBAinv, sizeof(accBAinv));
	LOG_INF("Calibrating main accelerometer 6-side offset");
	LOG_INF("Rest the device on a stable surface");

	sensor_calibration_clear_6_side();
	int err = sensor_6_sideBias();
	if (err)
	{
		magneto_reset();
		if (err == -1)
			LOG_INF("Motion detected");
		accBAinv[0][0] = NAN; // invalidate calibration
	}
	else
	{
		sys_write(MAIN_ACC_6_BIAS_ID, &retained->accBAinv, accBAinv, sizeof(accBAinv));
		LOG_INF("Accelerometer matrix:");
		for (int i = 0; i < 3; i++)
			LOG_INF("%.5f %.5f %.5f %.5f", (double)accBAinv[0][i], (double)accBAinv[1][i], (double)accBAinv[2][i], (double)accBAinv[3][i]);
	}
	if (sensor_calibration_validate_6_side())
	{
		set_led(SYS_LED_PATTERN_OFF, SYS_LED_PRIORITY_SENSOR);
		LOG_INF("Restoring previous calibration");
		memcpy(accBAinv, last_accBAinv, sizeof(accBAinv)); // restore last calibration
		LOG_INF("Accelerometer matrix:");
		for (int i = 0; i < 3; i++)
			LOG_INF("%.5f %.5f %.5f %.5f", (double)accBAinv[0][i], (double)accBAinv[1][i], (double)accBAinv[2][i], (double)accBAinv[3][i]);
		sensor_calibration_validate_6_side(); // additionally verify old calibration
		return;
	}

	LOG_INF("Finished calibration");
	set_led(SYS_LED_PATTERN_ONESHOT_COMPLETE, SYS_LED_PRIORITY_SENSOR);
}
#endif

static float aBuf[3] = {0};
uint64_t accel_sample = 0;
uint64_t accel_wait_sample = 0;

static void sensor_sample_accel(const float a[3])
{
	memcpy(aBuf, a, sizeof(aBuf));
	accel_sample++;
	if (accel_wait_sample)
		k_usleep(1); // yield to waiting thread
}

static int sensor_wait_accel(float a[3], k_timeout_t timeout)
{
	int64_t sample_end_time = MAX(k_uptime_ticks() + timeout.ticks, timeout.ticks);
	accel_wait_sample = accel_sample;
	while (accel_sample <= accel_wait_sample && k_uptime_ticks() < sample_end_time)
		k_usleep(1);
	accel_wait_sample = 0;
	if (k_uptime_ticks() >= sample_end_time)
	{
		LOG_ERR("Accelerometer wait timed out");
		return -1;
	}
	memcpy(a, aBuf, sizeof(aBuf));
	return 0;
}

static float gBuf[3] = {0};
uint64_t gyro_sample = 0;
uint64_t gyro_wait_sample = 0;

static void sensor_sample_gyro(const float g[3])
{
	memcpy(gBuf, g, sizeof(gBuf));
	gyro_sample++;
	if (gyro_wait_sample)
		k_usleep(1); // yield to waiting thread
}

static int sensor_wait_gyro(float g[3], k_timeout_t timeout)
{
	int64_t sample_end_time = MAX(k_uptime_ticks() + timeout.ticks, timeout.ticks);
	gyro_wait_sample = gyro_sample;
	while (gyro_sample <= gyro_wait_sample && k_uptime_ticks() < sample_end_time)
		k_usleep(1);
	gyro_wait_sample = 0;
	if (k_uptime_ticks() >= sample_end_time)
	{
		LOG_ERR("Gyroscope wait timed out");
		return -1;
	}
	memcpy(g, gBuf, sizeof(gBuf));
	return 0;
}

static float mBuf[3] = {0};
uint64_t mag_sample = 0;
uint64_t mag_wait_sample = 0;

static void sensor_sample_mag(const float m[3])
{
	memcpy(mBuf, m, sizeof(mBuf));
	mag_sample++;
	if (mag_wait_sample)
		k_usleep(1); // yield to waiting thread
}

static int sensor_wait_mag(float m[3], k_timeout_t timeout)
{
	int64_t sample_end_time = MAX(k_uptime_ticks() + timeout.ticks, timeout.ticks);
	mag_wait_sample = mag_sample;
	while (mag_sample <= mag_wait_sample && k_uptime_ticks() < sample_end_time)
		k_usleep(1);
	mag_wait_sample = 0;
	if (k_uptime_ticks() >= sample_end_time)
	{
		LOG_ERR("Magnetometer wait timed out");
		return -1;
	}
	memcpy(m, mBuf, sizeof(mBuf));
	return 0;
}

static int check_sides(const float *a)
{
	return (-1.2f < a[0] && a[0] < -0.8f ? 1 << 0 : 0) | (1.2f > a[0] && a[0] > 0.8f ? 1 << 1 : 0) | // dumb check if all accel axes were reached for calibration, assume the user is intentionally doing this
		(-1.2f < a[1] && a[1] < -0.8f ? 1 << 2 : 0) | (1.2f > a[1] && a[1] > 0.8f ? 1 << 3 : 0) |
		(-1.2f < a[2] && a[2] < -0.8f ? 1 << 4 : 0) | (1.2f > a[2] && a[2] > 0.8f ? 1 << 5 : 0);
}

// TODO: terrible name
static void sensor_sample_mag_magneto_sample(const float a[3], const float m[3])
{
	magneto_sample(m[0], m[1], m[2], ata, &norm_sum, &sample_count); // 400us
	uint8_t new_mag_progress = mag_progress;
	new_mag_progress |= check_sides(a);
	if (new_mag_progress > mag_progress && new_mag_progress == last_mag_progress)
	{
		if (k_uptime_get() > mag_progress_time)
		{
			mag_progress = new_mag_progress;
			LOG_INF("Magnetometer calibration progress: %s %s %s %s %s %s" , (new_mag_progress & 0x01) ? "-X" : "--", (new_mag_progress & 0x02) ? "+X" : "--", (new_mag_progress & 0x04) ? "-Y" : "--", (new_mag_progress & 0x08) ? "+Y" : "--", (new_mag_progress & 0x10) ? "-Z" : "--", (new_mag_progress & 0x20) ? "+Z" : "--");
			set_led(SYS_LED_PATTERN_ONESHOT_PROGRESS, SYS_LED_PRIORITY_SENSOR);
		}
	}
	else
	{
		mag_progress_time = k_uptime_get() + 1000;
		last_mag_progress = new_mag_progress;
	}
	if (mag_progress == 0b10111111)
		set_led(SYS_LED_PATTERN_FLASH, SYS_LED_PRIORITY_SENSOR); // Magnetometer calibration is ready to apply
}

int sensor_calibrate_mag(void)
{
	float zero[3] = {0};
	if (v_diff_mag(magBAinv[0], zero) != 0)
		return -1; // magnetometer calibration already exists

	float m[3];
	if (sensor_wait_mag(m, K_MSEC(1000)))
		return -1; // Timeout
	sensor_sample_mag_magneto_sample(aBuf, m); // 400us
	if (mag_progress != 0b11111111)
		return 0;

	float last_magBAinv[4][3];
	memcpy(last_magBAinv, magBAinv, sizeof(magBAinv));
	LOG_INF("Calibrating magnetometer hard/soft iron offset");

	// max allocated 1072 bytes
	magneto_current_calibration(magBAinv, ata, norm_sum, sample_count); // 25ms
	mag_progress = 0;
	// clear data
	memset(ata, 0, sizeof(ata));
	norm_sum = 0.0;
	sample_count = 0.0;
	sys_write(MAIN_MAG_BIAS_ID, &retained->magBAinv, magBAinv, sizeof(magBAinv));
	LOG_INF("Magnetometer matrix:");
	for (int i = 0; i < 3; i++)
		LOG_INF("%.5f %.5f %.5f %.5f", (double)magBAinv[0][i], (double)magBAinv[1][i],(double)magBAinv[2][i], (double)magBAinv[3][i]);
	if (sensor_calibration_validate_mag())
	{
		set_led(SYS_LED_PATTERN_OFF, SYS_LED_PRIORITY_SENSOR);
		LOG_INF("Restoring previous calibration");
		memcpy(magBAinv, last_magBAinv, sizeof(magBAinv)); // restore last calibration
		LOG_INF("Magnetometer matrix:");
		for (int i = 0; i < 3; i++)
			LOG_INF("%.5f %.5f %.5f %.5f", (double)magBAinv[0][i], (double)magBAinv[1][i],(double)magBAinv[2][i], (double)magBAinv[3][i]);
		sensor_calibration_validate_mag(); // additionally verify old calibration
		return -1;
	}

	LOG_INF("Finished calibration");
	set_led(SYS_LED_PATTERN_ONESHOT_COMPLETE, SYS_LED_PRIORITY_SENSOR);
	return 0;
}

int sensor_calibration_validate(void)
{
	float zero[3] = {0};
	if (!v_epsilon(accelBias, zero, 0.5) || !v_epsilon(gyroBias, zero, 50.0)) // check accel is <0.5G and gyro <50dps
	{
		sensor_calibration_clear();
		LOG_WRN("Invalidated calibration");
		LOG_WRN("The IMU may be damaged or calibration was not completed properly");
		return -1;
	}
	return 0;
}

int sensor_calibration_validate_6_side(void)
{
	float zero[3] = {0};
	float diagonal[3];
	for (int i = 0; i < 3; i++)
		diagonal[i] = accBAinv[i + 1][i];
	float magnitude = v_avg(diagonal);
	float average[3] = {magnitude, magnitude, magnitude};
	if (!v_epsilon(accBAinv[0], zero, 0.5) || !v_epsilon(diagonal, average, magnitude * 0.1f)) // check accel is <0.5G and diagonals are within 10%
	{
		sensor_calibration_clear_6_side();
		LOG_WRN("Invalidated calibration");
		LOG_WRN("The IMU may be damaged or calibration was not completed properly");
		return -1;
	}
	return 0;
}

int sensor_calibration_validate_mag(void)
{
	float zero[3] = {0};
	float diagonal[3];
	for (int i = 0; i < 3; i++)
		diagonal[i] = magBAinv[i + 1][i];
	float magnitude = v_avg(diagonal);
	float average[3] = {magnitude, magnitude, magnitude};
	if (!v_epsilon(magBAinv[0], zero, 1) || !v_epsilon(diagonal, average, MAX(magnitude * 0.2f, 0.1f))) // check offset is <1 unit and diagonals are within 20%
	{
		sensor_calibration_clear_mag();
		LOG_WRN("Invalidated calibration");
		LOG_WRN("The magnetometer may be damaged or calibration was not completed properly");
		return -1;
	}
	return 0;
}

void sensor_calibration_clear(void)
{
	memset(accelBias, 0, sizeof(accelBias));
	memset(gyroBias, 0, sizeof(gyroBias));
	sys_write(MAIN_ACCEL_BIAS_ID, &retained->accelBias, accelBias, sizeof(accelBias));
	sys_write(MAIN_GYRO_BIAS_ID, &retained->gyroBias, gyroBias, sizeof(gyroBias));

	sensor_fusion_invalidate();
}

#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
void sensor_calibration_clear_6_side(void)
{
	memset(accBAinv, 0, sizeof(accBAinv));
	for (int i = 0; i < 3; i++) // set identity matrix
		accBAinv[i + 1][i] = 1;
	sys_write(MAIN_ACC_6_BIAS_ID, &retained->accBAinv, accBAinv, sizeof(accBAinv));
}
#endif

void sensor_calibration_clear_mag(void)
{
	memset(magBAinv, 0, sizeof(magBAinv)); // zeroed matrix will disable magnetometer in fusion
	sys_write(MAIN_MAG_BIAS_ID, &retained->magBAinv, magBAinv, sizeof(magBAinv));
}

static int sensor_calibration_request(int id)
{
	static int requested = 0;
	switch (id)
	{
	case -1:
		requested = 0;
		return 0;
	case 0:
		return requested;
	default:
		if (requested != 0)
		{
			LOG_ERR("Sensor calibration is already running");
			return -1;
		}
		requested = id;
		return 0;
	}
}

void sensor_request_calibration(void)
{
	sensor_calibration_request(1);
}

#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
void sensor_request_calibration_6_side(void)
{
	sensor_calibration_request(2);
}
#endif

void sensor_request_calibration_mag(void)
{
	mag_progress |= 1 << 7;
	if (mag_progress == 0b10111111)
		mag_progress |= 1 << 6;
}

// TODO: setup 6 sided calibration (bias and scale, and maybe gyro ZRO?), setup temp calibration (particulary for gyro ZRO)
int sensor_offsetBias(float *dest1, float *dest2)
{
	float rawData[3], last_a[3];
	if (sensor_wait_accel(last_a, K_MSEC(1000)))
		return -2; // Timeout
	int64_t sampling_start_time = k_uptime_get();
	int i = 0;
	while (k_uptime_get() < sampling_start_time + 3000)
	{
		if (sensor_wait_accel(rawData, K_MSEC(1000)))
			return -2; // Timeout
		if (!v_epsilon(rawData, last_a, 0.1))
			return -1; // Motion detected
#if !CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
		dest1[0] += rawData[0];
		dest1[1] += rawData[1];
		dest1[2] += rawData[2];
#endif
		if (sensor_wait_gyro(rawData, K_MSEC(1000)))
			return -2; // Timeout
		dest2[0] += rawData[0];
		dest2[1] += rawData[1];
		dest2[2] += rawData[2];
		i++;
	}
	LOG_INF("Samples: %d", i);
#if !CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
	dest1[0] /= i;
	dest1[1] /= i;
	dest1[2] /= i;
	if (dest1[0] > 0.9f)
		dest1[0] -= 1.0f; // Remove gravity from the x-axis accelerometer bias calculation
	else if (dest1[0] < -0.9f)
		dest1[0] += 1.0f; // Remove gravity from the x-axis accelerometer bias calculation
	else if (dest1[1] > 0.9f)
		dest1[1] -= 1.0f; // Remove gravity from the y-axis accelerometer bias calculation
	else if (dest1[1] < -0.9f)
		dest1[1] += 1.0f; // Remove gravity from the y-axis accelerometer bias calculation
	else if (dest1[2] > 0.9f)
		dest1[2] -= 1.0f; // Remove gravity from the z-axis accelerometer bias calculation
	else if (dest1[2] < -0.9f)
		dest1[2] += 1.0f; // Remove gravity from the z-axis accelerometer bias calculation
	else
		return -1;
#endif
	dest2[0] /= i;
	dest2[1] /= i;
	dest2[2] /= i;
	return 0;
}

#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
static int isAccRest(float *acc, float *pre_acc, float threshold, int *t, int restdelta)
{
	float delta_x = acc[0] - pre_acc[0];
	float delta_y = acc[1] - pre_acc[1];
	float delta_z = acc[2] - pre_acc[2];

	float norm_diff = sqrt(delta_x * delta_x + delta_y * delta_y + delta_z * delta_z);

	if (norm_diff <= threshold)
		*t += restdelta;
	else
		*t = 0;

	if (*t > 2000)
		return 1;
	return 0;
}

static void magneto_reset(void)
{	
	mag_progress = 0; // reusing ata, so guarantee cleared mag progress
	last_mag_progress = 0;
	mag_progress_time = 0;
	memset(ata, 0, sizeof(ata));
	norm_sum = 0;
	sample_count = 0;
}

// TODO: can be used to get a better gyro bias
int sensor_6_sideBias(void)
{
	// Acc 6 side calibrate
	float rawData[3];
	float pre_acc[3] = {0};

	const float THRESHOLD_ACC = 0.05;
	int resttime = 0;

	magneto_reset();
	int c = 0;
	printk("Starting accelerometer calibration.\n");
	while (1)
	{
		set_led(SYS_LED_PATTERN_LONG, SYS_LED_PRIORITY_SENSOR);
		printk("Waiting for a resting state...\n");
		while (1)
		{
			if (sensor_wait_accel(rawData, K_MSEC(1000)))
				return -2; // Timeout, magneto state not handled here
			int rest = isAccRest(rawData, pre_acc, THRESHOLD_ACC, &resttime, 100);
			pre_acc[0] = rawData[0];
			pre_acc[1] = rawData[1];
			pre_acc[2] = rawData[2];

			// force not resting until a new side is detected and stable
			uint8_t new_mag_progress = mag_progress;
			new_mag_progress |= check_sides(rawData);
			if (new_mag_progress > mag_progress && new_mag_progress == last_mag_progress)
			{
				if (k_uptime_get() < mag_progress_time)
					rest = 0;
			}
			else
			{
				mag_progress_time = k_uptime_get() + 1000;
				last_mag_progress = new_mag_progress;
				rest = 0;
			}

			if (rest == 1)
			{
				mag_progress = new_mag_progress;
				printk("Rest detected, starting recording. Please do not move. %d\n", c);
				set_led(SYS_LED_PATTERN_ON, SYS_LED_PRIORITY_SENSOR);
				k_msleep(100);

				int64_t sampling_start_time = k_uptime_get();
				uint8_t i = 0;
				while (k_uptime_get() < sampling_start_time + 1000)
				{
					if (sensor_wait_accel(rawData, K_MSEC(1000)))
						return -2; // Timeout, magneto state not handled here
					if (!v_epsilon(rawData, pre_acc, 0.1))
						return -1; // Motion detected
					magneto_sample(rawData[0], rawData[1], rawData[2], ata, &norm_sum, &sample_count);
					if (k_uptime_get() >= sampling_start_time + i * 100)
					{
						printk("#");
						i++;
					}
				}
				set_led(SYS_LED_PATTERN_ONESHOT_PROGRESS, SYS_LED_PRIORITY_SENSOR);
				printk("Recorded values!\n");
				printk("%d side done\n", c);
				c++;
				break;
			}
			k_msleep(100);
		}
		if(c >= 6) break;
		printk("Waiting for the next side... %d \n", c);
		while (1)
		{
			k_msleep(100);
			if (sensor_wait_accel(rawData, K_MSEC(1000)))
				return -2; // Timeout, magneto state not handled here
			int rest = isAccRest(rawData, pre_acc, THRESHOLD_ACC, &resttime, 100);
			pre_acc[0] = rawData[0];
			pre_acc[1] = rawData[1];
			pre_acc[2] = rawData[2];

			if (rest == 0)
			{
				resttime = 0;
				break;
			}
		}
	}

	printk("Calculating the data....\n");
	magneto_current_calibration(accBAinv, ata, norm_sum, sample_count);
	magneto_reset();

	printk("Calibration is complete.\n");
	return 0;
}
#endif

static void calibration_thread(void)
{
	sensor_calibration_read();
	// TODO: be able to block the sensor while doing certain operations
	// TODO: reset fusion on calibration finished
	// TODO: start and run thread from request?
	// TODO: replace wait_for_motion with isAccRest

	// Verify calibrations
	sensor_calibration_validate();
#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
	sensor_calibration_validate_6_side();
#endif
	sensor_calibration_validate_mag();

	// requested calibrations run here
	while (1)
	{
		int requested = sensor_calibration_request(0);
		switch (requested)
		{
		case 1:
			set_status(SYS_STATUS_CALIBRATION_RUNNING, true);
			sensor_calibrate_imu();
			sensor_calibration_request(-1); // clear request
			set_status(SYS_STATUS_CALIBRATION_RUNNING, false);
			break;
#if CONFIG_SENSOR_USE_6_SIDE_CALIBRATION
		case 2:
			set_status(SYS_STATUS_CALIBRATION_RUNNING, true);
			sensor_calibrate_6_side();
			sensor_calibration_request(-1); // clear request
			set_status(SYS_STATUS_CALIBRATION_RUNNING, false);
			break;
#endif
		default:
			if (mag_progress & 0b10000000)
				requested = sensor_calibrate_mag();
			break;
		}
		if (requested < 0)
			k_msleep(5);
		else
			k_msleep(100);
	}
}
