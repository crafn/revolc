#ifndef REVOLC_ANIMATION_CLIPINST_H
#define REVOLC_ANIMATION_CLIPINST_H

#include "build.h"
#include "clip.h"
#include "joint.h"

// Animation clip being played
typedef struct ClipInst {
	// @todo Use ResId instead
	char clip_name[RES_NAME_SIZE];

	JointPoseArray pose; // Interpolated straight from Clip
	F32 t;
} ClipInst;

REVOLC_API
void init_clipinst(ClipInst *data);

// In-place resurrection
REVOLC_API
U32 resurrect_clipinst(ClipInst *dead);

REVOLC_API
void upd_clipinst(ClipInst *inst, ClipInst *e);

#endif // REVOLC_ANIMATION_CLIPINST_H
