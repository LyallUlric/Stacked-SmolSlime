#ifndef SLIMENRF_FUSION_NONE
#define SLIMENRF_FUSION_NONE

#include "sensor/sensor.h"

void fusion_none_init(float g_time, float a_time, float m_time);
void fusion_none_load(const void *data);
void fusion_none_save(void *data);

void fusion_none_update_gyro(float *g, float time);
void fusion_none_update_accel(float *a, float time);
void fusion_none_update_mag(float *m, float time);
void fusion_none_update(float *g, float *a, float *m, float time);

void fusion_none_get_gyro_bias(float *g_off);
void fusion_none_set_gyro_bias(float *g_off);

void fusion_none_update_gyro_sanity(float *g, float *m);
int fusion_none_get_gyro_sanity(void);

void fusion_none_get_lin_a(float *lin_a);
void fusion_none_get_quat(float *q);

extern const sensor_fusion_t sensor_fusion_none;

#endif
