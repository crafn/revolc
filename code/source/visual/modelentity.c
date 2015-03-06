#include "modelentity.h"

void init_modelentity(ModelEntity *data)
{
	*data= (ModelEntity) {
		.tf= {
			.rot= identity_qd(),
			.scale= {1, 1, 1},
		}
	};
}
