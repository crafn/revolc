#ifndef REVOLC_VISUAL_COMPDEF_H
#define REVOLC_VISUAL_COMPDEF_H

#include "build.h"
#include "resources/resource.h"

typedef struct CompDef_Sub {
	char entity_name[RES_NAME_SIZE];
	char joint_name[RES_NAME_SIZE];
	/// @todo Shouldn't we cache joint id for fast CompEntity creation?
	T3f offset;
} CompDef_Sub;

// Definition for CompEntity
// CompEntity and CompDef are analoguous with ModelEntity and Model
typedef struct CompDef {
	Resource res;
	char armature_name[RES_NAME_SIZE];

	CompDef_Sub subs[MAX_SUBENTITY_COUNT];
	U32 sub_count;
} PACKED CompDef;

REVOLC_API WARN_UNUSED
int json_compdef_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API WARN_UNUSED
CompDef *blobify_compdef(struct WArchive *ar, Cson c, bool *err);

#endif // REVOLC_VISUAL_COMPDEF_H
