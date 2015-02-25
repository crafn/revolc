#ifndef REVOLC_PLATFORM_DEVICE_H
#define REVOLC_PLATFORM_DEVICE_H

#include "build.h"

#define KEYBOARD_KEY_COUNT 256
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

struct DevicePlatformData;
typedef struct Device {
	int cursor_pos[2];
	int win_size[2];
	bool quit_requested;
	F32 dt;
	bool lmbDown;

	bool keyDown[KEYBOARD_KEY_COUNT];
	bool keyPressed[KEYBOARD_KEY_COUNT];
	bool keyReleased[KEYBOARD_KEY_COUNT];

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
