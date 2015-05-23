#ifndef REVOLC_PLATFORM_DEVICE_H
#define REVOLC_PLATFORM_DEVICE_H

#include "build.h"
#include "core/vector.h"

#define KEY_COUNT 256
#define KEY_LMB 0
#define KEY_MMB 1
#define KEY_RMB 2
#define KEY_TAB 9
#define KEY_LSHIFT 10
#define KEY_LCTRL 11
#define KEY_SPACE 12
#define KEY_DEL 13
#define KEY_KP_0 14
#define KEY_KP_1 15
#define KEY_KP_2 16
#define KEY_KP_3 17
#define KEY_KP_4 18
#define KEY_KP_5 19
#define KEY_KP_6 20
#define KEY_KP_7 21
#define KEY_KP_8 22
#define KEY_KP_9 23
// @todo Change to match with ascii
#define KEY_0 24
#define KEY_1 25
#define KEY_2 26
#define KEY_3 27
#define KEY_4 28
#define KEY_5 29
#define KEY_6 30
#define KEY_7 31
#define KEY_8 32
#define KEY_9 33

// ...

#define KEY_A 97 // 'a'
//  ...
#define KEY_F1  128
#define KEY_F2  129
#define KEY_F3  130
#define KEY_F4  131
#define KEY_F5  132
#define KEY_F6  133
#define KEY_F7  134
#define KEY_F8  135
#define KEY_F9  136
#define KEY_F10 137
#define KEY_F11 138
#define KEY_F12 139
#define KEY_ESC 140
#define KEY_LEFT 144
#define KEY_UP 145
#define KEY_RIGHT 146
#define KEY_DOWN 147

struct DevicePlatformData;
typedef struct Device {
	V2i cursor_pos;
	V2i win_size;
	bool quit_requested;
	F32 dt;

	bool key_down[KEY_COUNT];
	bool key_pressed[KEY_COUNT];
	bool key_released[KEY_COUNT];

	F64 mwheel_delta;

	struct DevicePlatformData* impl;
} Device;

/// @note Sets g_env.device
REVOLC_API Device * plat_init(const char* title, V2i reso);
REVOLC_API void plat_quit(Device *d);

REVOLC_API void plat_update(Device *d);
REVOLC_API void plat_sleep(int ms);
REVOLC_API void plat_flush_denormals(bool enable);

/// @return Mallocated null-terminated array of null-terminated, mallocated strings
REVOLC_API char ** plat_find_paths_with_end(const char *path_to_dir, const char *end);

#endif // REVOLC_PLATFORM_DEVICE_H
