#include "modelentity.h"

void init_modelentity(ModelEntity *data)
{
	*data= (ModelEntity) {
		.rot= {1, 0},
		.scale= {1, 1, 1},
	};
}
