#include "core/archive.h"
#include "modelentity.h"

void upd_smoothing_phase(F32 *phase, F64 dt)
{
	if (*phase < 0.001f) {
		*phase = 0.0f;
		return;
	}

	*phase = exp_drive(*phase, 0, dt*15);
}

T3d smoothed_tf(T3d tf, F32 phase, T3d delta)
{
	if (phase == 0.0f)
		return tf;

	T3d ret = lerp_t3d(tf, mul_t3d(tf, delta), phase);
	ret.rot = normalized_qd(ret.rot);
	return ret;
}

void init_modelentity(ModelEntity *data)
{
	*data = (ModelEntity) {
		.tf = identity_t3d(),
		.color = (Color) {1, 1, 1, 1},
	};
}

void pack_modelentity(WArchive *ar, const ModelEntity *begin, const ModelEntity *end)
{
	for (const ModelEntity *it = begin; it != end; ++it) {
		ensure(!it->has_own_mesh && "@todo");
		pack_strbuf(ar, it->model_name, sizeof(it->model_name));
		lossy_pack_t3d(ar, &it->tf);
		pack_s32(ar, &it->layer);
	}
}

void unpack_modelentity(RArchive *ar, ModelEntity *begin, ModelEntity *end)
{
	for (ModelEntity *it = begin; it != end; ++it) {
		init_modelentity(it);

		unpack_strbuf(ar, it->model_name, sizeof(it->model_name));
		lossy_unpack_t3d(ar, &it->tf);
		unpack_s32(ar, &it->layer);
	}
}
