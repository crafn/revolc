#ifndef REVOLC_RESOURCES_RESOURCE_H
#define REVOLC_RESOURCES_RESOURCE_H

#include "build.h"

typedef U64 ResOffset; // Offset from the beginning of a resource blob
#define RES_NAME_LEN 16

typedef enum {
#define RESOURCE(x) ResType_ ## x,
#	include "resources.def"
#undef RESOURCE
} ResType;

typedef struct {
	ResType type;
	char name[RES_NAME_LEN];
} PACKED Resource;

#endif // REVOLC_RESOURCES_RESOURCE_H
