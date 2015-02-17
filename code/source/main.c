#include <stdio.h>

#include "build.h"
#include "core/ensure.h"
#include "core/vector.h"
#include "platform/device.h"
#include "platform/gl.h"

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
		F32 c_gl[2]= {
			2.0*d.cursor_pos[0]/d.win_size[0] - 1.0,
			-2.0*d.cursor_pos[1]/d.win_size[1] + 1.0,
		};

		glViewport(0, 0, d.win_size[0], d.win_size[1]);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3f(1.0, 0.0, 1.0);
		glLoadIdentity();
		glBegin(GL_QUADS);
			glVertex2f(0.0 + c_gl[0], 0.0 + c_gl[1]);
			glVertex2f(1.0 + c_gl[0], 0.0 + c_gl[1]);
			glVertex2f(1.0 + c_gl[0], 1.0 + c_gl[1]);
			glVertex2f(0.0 + c_gl[0], 1.0 + c_gl[1]);
		glEnd();

		gl_check_errors("loop");

		plat_sleep(1);
	}

	plat_quit(&d);
	return 0;
}
