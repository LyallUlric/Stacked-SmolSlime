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
#include "util.h"

#include "sensor/sensors_enum.h"
#include "../src/vqf.h" // conflicting with vqf.h in local path

#include "../vqf/vqf.h" // conflicting with vqf.h in vqf-c

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (M_PI / 180.0f)
#endif

static uint8_t imu_id;

static vqf_params_t params;
static vqf_state_t state;
static vqf_coeffs_t coeffs;

static float last_a[3] = {0};

void vqf_update_sensor_ids(int imu)
{
	imu_id = imu;
}

static void set_params()
{
	init_params(&params);
	params.biasClip = 5.0f;
	params.tauAcc = 4.5f; // best result for VQF from paper
	params.tauMag = 10.0f; // best result for VQF from paper	
	// optimized with ICM-45686 data, (in parentheses based on BMI270 characteristics)
	if (imu_id == IMU_ICM45686 || imu_id == IMU_LSM6DSR)
	{
		params.tauAcc = 6.5f; // best result from Lyall (best result from optimizer: 7.171490)
//		params.tauMag = 10.0f; // best result for VQF from paper	
		params.biasSigmaInit = 0.337976f; // best result from optimizer (based on gyro ZRO temperature drift: 0.25)
		params.biasForgettingTime = 352.235500f; // best result from optimizer (based on time for temperature change: 200)
//		params.biasClip = 5.0f; // globally set (based on gyro ZRO: 1.0f)
		params.biasSigmaMotion = 0.985346f; // best result from optimizer
		params.biasVerticalForgettingFactor = 0.007959f; // best result from optimizer
		params.biasSigmaRest = 0.028897f; // best result from optimizer (changed, unknown reason: 0.01f)
		params.restMinT = 4.648680f; // best result from optimizer
		params.restFilterTau = 1.900166f; // best result from optimizer
		params.restThGyr = 2.620598f; // best result from optimizer (based on gyro noise: 0.5f)
		params.restThAcc = 2.142593f; // best result from optimizer (based on accel noise: 0.1f)
//		params.magCurrentTau = 0.05f;
//		params.magRefTau = 20.0f;
//		params.magNormTh = 0.1f;
//		params.magDipTh = 10.0f;
//		params.magNewTime = 20.0f;
//		params.magNewFirstTime = 5.0f;
//		params.magNewMinGyr = 20.0f;
//		params.magMinUndisturbedTime = 0.5f;
//		params.magMaxRejectionTime = 60.0f;
//		params.magRejectionFactor = 2.0f;
	}
	// Per-IMU parameters, from ESP firmware
	switch (imu_id)
	{
	case IMU_BMI270:
	case IMU_ICM42688:
		params.biasSigmaInit = 0.5f;
		// params.biasClip = 1.0f;
		params.restThGyr = 0.5f;
		params.restThAcc = 0.196f;
		break;
//	case IMU_ICM45686:
		// params.biasClip = 1.0f;
//		break;
	case IMU_LSM6DS3:
		params.biasSigmaInit = 3.0f;
		// params.biasClip = 6.0f;
		params.restThGyr = 3.0f;
		params.restThAcc = 0.392f;
		break;
	case IMU_LSM6DSO:
//	case IMU_LSM6DSR:
	case IMU_LSM6DSV:
		params.biasSigmaInit = 1.0f;
		// params.biasClip = 2.0f;
		params.restThGyr = 1.0f;
		params.restThAcc = 0.192f;
		break;
	default:
	}
}

void vqf_init(float g_time, float a_time, float m_time)
{
	set_params();
	initVqf(&params, &state, &coeffs, g_time, a_time, m_time);
}

void vqf_load(const void *data)
{
	set_params();
	memcpy(&state, data, sizeof(state));
	memcpy(&coeffs, (uint8_t *)data + sizeof(state), sizeof(coeffs));
}

void vqf_save(void *data)
{
	memcpy(data, &state, sizeof(state));
	memcpy((uint8_t *)data + sizeof(state), &coeffs, sizeof(coeffs));
}

void vqf_update_gyro(float *g, float time)
{
	// TODO: time unused?
	float g_rad[3] = {0};
	// g is in deg/s, convert to rad/s
	for (int i = 0; i < 3; i++)
		g_rad[i] = g[i] * DEG_TO_RAD;
	updateGyr(&params, &state, &coeffs, g_rad);
}

void vqf_update_accel(float *a, float time)
{
	// TODO: time unused?
	// TODO: how to handle change in sample rate
	float a_m_s2[3] = {0};
	// a is in g, convert to m/s^2
	for (int i = 0; i < 3; i++)
		a_m_s2[i] = a[i] * CONST_EARTH_GRAVITY;
	if (a_m_s2[0] != 0 || a_m_s2[1] != 0 || a_m_s2[2] != 0)
		memcpy(last_a, a_m_s2, sizeof(a_m_s2));
	updateAcc(&params, &state, &coeffs, a_m_s2);
}

void vqf_update_mag(float *m, float time)
{
	// TODO: time unused?
	updateMag(&params, &state, &coeffs, m);
}

void vqf_update(float *g, float *a, float *m, float time)
{
	// TODO: time unused?
	// TODO: gyro is a different rate to the others, should they be separated
	if (g[0] != 0 || g[1] != 0 || g[2] != 0) // ignore zeroed gyro
		vqf_update_gyro(g, time);
	vqf_update_accel(a, time);
	vqf_update_mag(m, time);
}

void vqf_get_gyro_bias(float *g_off)
{
	getBiasEstimate(&state, &coeffs, g_off);
}

void vqf_set_gyro_bias(float *g_off)
{
	setBiasEstimate(&state, g_off, -1);
}

void vqf_update_gyro_sanity(float *g, float *m)
{
	// TODO: does vqf tell us a "recovery state"
	return;
}

int vqf_get_gyro_sanity(void)
{
	// TODO: does vqf tell us a "recovery state"
	return 0;
}

void vqf_get_lin_a(float *lin_a)
{
	float q[4] = {0};
	vqf_get_quat(q);

	float vec_gravity[3] = {0};
	vec_gravity[0] = 2.0f * (q[1] * q[3] - q[0] * q[2]);
	vec_gravity[1] = 2.0f * (q[2] * q[3] + q[0] * q[1]);
	vec_gravity[2] = 2.0f * (q[0] * q[0] - 0.5f + q[3] * q[3]);

//	float *a = state.lastAccLp; // not usable, rotated by inertial frame
	float *a = last_a;
	for (int i = 0; i < 3; i++)
		lin_a[i] = a[i] - vec_gravity[i] * CONST_EARTH_GRAVITY; // gravity vector to m/s^2 before subtracting
}

void vqf_get_quat(float *q)
{
	getQuat9D(&state, q);
}

bool vqf_get_rest_detected(void)
{
	return getRestDetected(&state);
}

void vqf_get_relative_rest_deviations(float *out)
{
	getRelativeRestDeviations(&params, &state, out);
}

const sensor_fusion_t sensor_fusion_vqf = {
	*vqf_init,
	*vqf_load,
	*vqf_save,

	*vqf_update_gyro,
	*vqf_update_accel,
	*vqf_update_mag,
	*vqf_update,

	*vqf_get_gyro_bias,
	*vqf_set_gyro_bias,

	*vqf_update_gyro_sanity,
	*vqf_get_gyro_sanity,

	*vqf_get_lin_a,
	*vqf_get_quat
};
