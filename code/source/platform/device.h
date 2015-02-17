#ifndef REVOLC_PLATFORM_DEVICE_H
#define REVOLC_PLATFORM_DEVICE_H

#include "build.h"

struct DevicePlatformData;
typedef struct {
	int cursor_pos[2];
	int win_size[2];
	Bool quit_requested;
	F32 dt;

	struct DevicePlatformData* data;
} Device;

Device plat_init(const char* title, int width, int height);
void plat_quit(Device *d);

void plat_update(Device *d);
void plat_sleep(int ms);

#endif // REVOLC_PLATFORM_DEVICE_H
