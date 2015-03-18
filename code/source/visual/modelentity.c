#include "modelentity.h"

void init_modelentity(ModelEntity *data)
{
	*data= (ModelEntity) {
		.tf= identity_t3d(),
		.color= (Color) {1, 1, 1, 1},
	};
}
