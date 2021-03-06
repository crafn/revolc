#include "resource.h"

ResType str_to_restype(const char *str)
{
#define RESOURCE(x, init, deinit, blobify, deblobify, recache) \
	if (!strcmp(#x, str)) \
		return ResType_ ## x;
#	include "resources.def"
#undef RESOURCE
	return ResType_None;
}

const char * restype_to_str(ResType type)
{
#define RESOURCE(x, init, deinit, blobify, deblobify, recache) \
	if (ResType_ ## x == type) \
		return #x;
#	include "resources.def"
#undef RESOURCE
	return "None";
}
