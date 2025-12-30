#include <zephyr/logging/log.h>

#include "fusion_none.h"

LOG_MODULE_REGISTER(fusion_none, LOG_LEVEL_INF);

void fusion_none_init(float g_time, float a_time, float m_time)
{
	LOG_DBG("fusion_none_init");
}

void fusion_none_load(const void *data)
{
	LOG_DBG("fusion_none_load");
}

void fusion_none_save(void *data)
{
	LOG_DBG("fusion_none_save");
}

void fusion_none_update_gyro(float *g, float time)
{
	LOG_DBG("fusion_none_update_gyro");
}

void fusion_none_update_accel(float *a, float time)
{
	LOG_DBG("fusion_none_update_accel");
}

void fusion_none_update_mag(float *m, float time)
{
	LOG_DBG("fusion_none_update_mag");
}

void fusion_none_update(float *g, float *a, float *m, float time)
{
	LOG_DBG("fusion_none_update");
}

void fusion_none_get_gyro_bias(float *g_off)
{
	LOG_DBG("fusion_none_get_gyro_bias");
}

void fusion_none_set_gyro_bias(float *g_off)
{
	LOG_DBG("fusion_none_set_gyro_bias");
}

void fusion_none_update_gyro_sanity(float *g, float *m)
{
	LOG_DBG("fusion_none_update_gyro_sanity");
}

int fusion_none_get_gyro_sanity(void)
{
	LOG_DBG("fusion_none_get_gyro_sanity");
	return 0;
}

void fusion_none_get_lin_a(float *lin_a)
{
	LOG_DBG("fusion_none_get_lin_a");
}

void fusion_none_get_quat(float *q)
{
	LOG_DBG("fusion_none_get_quat");
}

const sensor_fusion_t sensor_fusion_none = {
	*fusion_none_init,
	*fusion_none_load,
	*fusion_none_save,

	*fusion_none_update_gyro,
	*fusion_none_update_accel,
	*fusion_none_update_mag,
	*fusion_none_update,

	*fusion_none_get_gyro_bias,
	*fusion_none_set_gyro_bias,

	*fusion_none_update_gyro_sanity,
	*fusion_none_get_gyro_sanity,

	*fusion_none_get_lin_a,
	*fusion_none_get_quat
};
