#ifndef REVOLC_PLATFORM_DEVICE_H
#define REVOLC_PLATFORM_DEVICE_H

#include "build.h"
#include "core/vector.h"

#define KEYBOARD_KEY_COUNT 256
#define KEY_LMB 0
#define KEY_RMB 2
#define KEY_LSHIFT 10
#define KEY_F1  127
#define KEY_F2  128
#define KEY_F3  129
#define KEY_F4  130
#define KEY_F5  131
#define KEY_F6  132
#define KEY_F7  133
#define KEY_F8  134
#define KEY_F9  135
#define KEY_F10 136
#define KEY_F11 137
#define KEY_F12 138
#define KEY_TAB 139

struct DevicePlatformData;
typedef struct Device {
	V2i cursor_pos;
	V2i win_size;
	bool quit_requested;
	F32 dt;

	bool key_down[KEYBOARD_KEY_COUNT];
	bool key_pressed[KEYBOARD_KEY_COUNT];
	bool key_released[KEYBOARD_KEY_COUNT];

	struct DevicePlatformData* data;
} Device;

/// @note Sets g_env.device
REVOLC_API Device * plat_init(const char* title, int width, int height);
REVOLC_API void plat_quit(Device *d);

REVOLC_API void plat_update(Device *d);
REVOLC_API void plat_sleep(int ms);
/// @return Mallocated null-terminated array of null-terminated, mallocated strings
REVOLC_API char ** plat_find_paths_with_end(const char *path_to_dir, const char *end);

#endif // REVOLC_PLATFORM_DEVICE_H
