#include <chipmunk/src/chipmunk.c>
#include <chipmunk/src/cpArbiter.c>
#include <chipmunk/src/cpArray.c>
#include <chipmunk/src/cpBBTree.c>
#include <chipmunk/src/cpBody.c>
#include <chipmunk/src/cpCollision.c>
#include <chipmunk/src/cpConstraint.c>
#include <chipmunk/src/cpDampedRotarySpring.c>
#include <chipmunk/src/cpDampedSpring.c>
#include <chipmunk/src/cpGearJoint.c>
#include <chipmunk/src/cpGrooveJoint.c>
#include <chipmunk/src/cpHashSet.c>
//#include <chipmunk/src/cpHastySpace.c> // Seems unnecessary, and uses pthreads :I
#include <chipmunk/src/cpMarch.c>
#include <chipmunk/src/cpPinJoint.c>
#include <chipmunk/src/cpPivotJoint.c>
#include <chipmunk/src/cpPolyline.c>
#include <chipmunk/src/cpPolyShape.c>
#include <chipmunk/src/cpRatchetJoint.c>
#include <chipmunk/src/cpRobust.c>
#include <chipmunk/src/cpRotaryLimitJoint.c>
#include <chipmunk/src/cpShape.c>
#include <chipmunk/src/cpSimpleMotor.c>
#include <chipmunk/src/cpSlideJoint.c>
#include <chipmunk/src/cpSpace.c>
#include <chipmunk/src/cpSpaceComponent.c>
#include <chipmunk/src/cpSpaceDebug.c>
#include <chipmunk/src/cpSpaceHash.c>
#include <chipmunk/src/cpSpaceQuery.c>
#include <chipmunk/src/cpSpaceStep.c>
#include <chipmunk/src/cpSpatialIndex.c>
#include <chipmunk/src/cpSweep1D.c>

#include <jsmn/jsmn.c>
#include <lodepng/lodepng.c>

#include <ogg/src/bitwise.c>
#include <ogg/src/framing.c>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include <vorbis/lib/analysis.c>
#include <vorbis/lib/bitrate.c>
#include <vorbis/lib/block.c>
#include <vorbis/lib/codebook.c>
#include <vorbis/lib/envelope.c>
#include <vorbis/lib/floor0.c>
#include <vorbis/lib/floor1.c>
#include <vorbis/lib/info.c>
#include <vorbis/lib/lookup.c>
#include <vorbis/lib/lpc.c>
#include <vorbis/lib/lsp.c>
#include <vorbis/lib/mapping0.c>
#include <vorbis/lib/mdct.c>
#include <vorbis/lib/psy.c>
#include <vorbis/lib/registry.c>
#include <vorbis/lib/res0.c>
#include <vorbis/lib/sharedbook.c>
#include <vorbis/lib/smallft.c>
#include <vorbis/lib/synthesis.c>
#include <vorbis/lib/vorbisenc.c>
#include <vorbis/lib/vorbisfile.c>
#include <vorbis/lib/window.c>

#include <miniz/miniz.c>

#define QC_BACKEND_C
//#define QC_MALLOC revolc_qc_malloc
//#define QC_FREE revolc_qc_free
#include <qc/lib_unity.c>
