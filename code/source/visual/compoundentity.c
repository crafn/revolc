#include "compoundentity.h"

void init_compoundentity(CompoundEntity *data)
{
	*data= (CompoundEntity) {
		.tf= identity_t3d(),
	};
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		data->joint_offsets.tf[i]= identity_t3f();
}
