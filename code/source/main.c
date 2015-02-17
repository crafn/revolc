#include <stdio.h>

#include "build.h"
#include "core/ensure.h"
#include "core/vector.h"
#include "platform/device.h"

int main()
{
	Device d= plat_init("Revolc engine", 800, 600);

	printf("Hello world!\n");
	V3f vec= {2, 3, 4};
	V3f r= add_V3f(vec, vec);
	printf("%f, %f, %f\n", r.x, r.y, r.z);
	printf("%f, %f, %f\n", r.e[0], r.e[1], r.e[2]);

	while (!d.quit_requested) {
		plat_update(&d);
		printf("cursor: %i, %i\n", d.cursor_pos[0], d.cursor_pos[1]);

		plat_sleep(100);
	}

	plat_quit(&d);
	return 0;
}
