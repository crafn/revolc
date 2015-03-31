#include "animation/armature.c"
#include "animation/clip.c"
#include "animation/clipinst.c"
#include "animation/joint.c"
#include "audio/audiosystem.c"
#include "audio/sound.c"
#include "core/array.c"
#include "core/debug_print.c"
#include "core/ensure.c"
#include "core/file.c"
#include "core/json.c"
#include "core/malloc.c"
#include "core/matrix.c"
#include "core/string.c"
#include "editor/armature_editor.c"
#include "editor/editor.c"
#include "editor/mesh_editor.c"
#include "editor/editor_util.c"
#include "game/aitest.c"
#include "game/nodegroupdef.c"
#include "game/nodetype.c"
#include "game/world.c"
#include "game/worldgen.c"
#include "global/env.c"
#include "global/module.c"
#include "global/rtti.c"
#include "main.c"
#include "physics/physmat.c"
#include "physics/physworld.c"
#include "physics/rigidbody.c"
#include "physics/rigidbodydef.c"
#include "platform/device.c"
#include "platform/file.c"
#include "platform/gl.c"
#include "platform/stdlib.c"
#include "resources/resblob.c"
#include "resources/resource.c"
#include "ui/uicontext.c"
#include "visual/compdef.c"
#include "visual/compentity.c"
#include "visual/ddraw.c"
#include "visual/font.c"
#include "visual/mesh.c"
#include "visual/model.c"
#include "visual/modelentity.c"
#include "visual/renderer.c"
#include "visual/shadersource.c"
#include "visual/texture.c"
#include "visual/vao.c"



#ifndef CODEGEN
#	if PLATFORM == PLATFORM_LINUX
#		include "platform/linux.c"
#	elif PLATFORM == PLATFORM_WINDOWS
#		include "platform/windows.c"
#	else
#		error "Unknown platform"
#	endif
#	include "global/gen_rtti.c"
#endif
