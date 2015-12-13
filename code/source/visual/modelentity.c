#include "core/archive.h"
#include "modelentity.h"

void init_modelentity(ModelEntity *data)
{
	*data = (ModelEntity) {
		.tf = identity_t3d(),
		.color = (Color) {1, 1, 1, 1},
	};
}

internal
void pack_t3d(WArchive *ar, const T3d *tf)
{
	pack_buf(ar, tf, sizeof(*tf));
}

internal
void unpack_t3d(RArchive *ar, T3d *tf)
{
	unpack_buf(ar, tf, sizeof(*tf));
}

void pack_modelentity(WArchive *ar, const ModelEntity *begin, const ModelEntity *end)
{
	for (const ModelEntity *it = begin; it != end; ++it) {
		ensure(!it->has_own_mesh && "@todo");
		pack_strbuf(ar, it->model_name, sizeof(it->model_name));
		pack_t3d(ar, &it->tf);
		pack_s32(ar, &it->layer);
	}
}

void unpack_modelentity(RArchive *ar, ModelEntity *begin, ModelEntity *end)
{
	for (ModelEntity *it = begin; it != end; ++it) {
		init_modelentity(it);

		unpack_strbuf(ar, it->model_name, sizeof(it->model_name));
		unpack_t3d(ar, &it->tf);
		unpack_s32(ar, &it->layer);
	}
}
