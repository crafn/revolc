#include "core/array.c"
#include "core/debug_print.c"
#include "core/ensure.c"
#include "core/json.c"
#include "core/malloc.c"
#include "global/env.c"
#include "main.c"
#include "platform/device.c"
#include "platform/gl.c"
#include "resources/resblob.c"
#include "resources/resource.c"
#include "visual/mesh.c"
#include "visual/model.c"
#include "visual/renderer.c"
#include "visual/shader.c"
#include "visual/texture.c"
#include "visual/vao.c"

/// @todo Separate compile for dependencies
#include <jsmn/jsmn.c>
#include <lodepng/lodepng.c>
