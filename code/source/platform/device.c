#include "device.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glx.h>
#include <time.h>
#include <X11/X.h>
#include <X11/Xlib.h>

typedef struct DevicePlatformData {
	Display* dpy;
	Window win;
	GLXContext ctx;
	struct timespec ts;
} DevicePlatformData;

internal
void plat_fail(const char *msg)
{
	printf("%s\n", msg);
	abort();
}

Device plat_init(const char* title, int width, int height)
{
	Device d= {};
	d.data= malloc(sizeof(*d.data));
	memset(d.data, 0, sizeof(*d.data));

	d.data->dpy= XOpenDisplay(NULL);
	if(d.data->dpy == NULL)
		plat_fail("XOpenDisplay failed");

	Window root= DefaultRootWindow(d.data->dpy);
	GLint att[]= { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	XVisualInfo* vi= glXChooseVisual(d.data->dpy, 0, att);

	if(vi == NULL)
		plat_fail("glXChooseVisual failed");

	Colormap cmap;
	cmap= XCreateColormap(d.data->dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap= cmap;
	swa.event_mask= ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask;
	d.data->win=
		XCreateWindow(	d.data->dpy,
						root,
						0, 0, width, height, 0,
						vi->depth,
						InputOutput,
						vi->visual,
						CWColormap | CWEventMask,
						&swa);
	XMapWindow(d.data->dpy, d.data->win);
	XStoreName(d.data->dpy, d.data->win, title);
	
	d.data->ctx= glXCreateContext(d.data->dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(d.data->dpy, d.data->win, d.data->ctx);

	clock_gettime(CLOCK_MONOTONIC, &d.data->ts);

	return d;
}

void plat_quit(Device *d)
{
	glXMakeCurrent(d->data->dpy, None, NULL);
	glXDestroyContext(d->data->dpy, d->data->ctx);
	XDestroyWindow(d->data->dpy, d->data->win);
	XCloseDisplay(d->data->dpy);

	free(d->data);
	d->data= NULL;
}

void plat_update(Device *d)
{
	glXSwapBuffers(d->data->dpy, d->data->win);

	while(XPending(d->data->dpy)) {
		XEvent xev;
        XNextEvent(d->data->dpy, &xev);
		if(xev.type == KeyPress) {
			int keys_ret;
			KeySym* keysym=
				XGetKeyboardMapping(d->data->dpy, xev.xkey.keycode, 1, &keys_ret);
			
			if (*keysym == XK_Escape)
				d->quit_requested= true;

			XFree(keysym);
		}

		/*if (xev.xbutton.type == ButtonPress)
			env.lmbDown= true;
		else if (xev.xbutton.type == ButtonRelease)
			env.lmbDown= false;*/
	}

	XWindowAttributes gwa;
	XGetWindowAttributes(d->data->dpy, d->data->win, &gwa);
	d->win_size[0]= gwa.width;
	d->win_size[1]= gwa.height;

	int root_x= 0, root_y= 0;
	Window w;
	int cursor_x, cursor_y;
	unsigned int mask;
	XQueryPointer(	d->data->dpy, d->data->win, &w,
					&w, &root_x, &root_y, &d->cursor_pos[0], &d->cursor_pos[1],
					&mask);

	long old_us= d->data->ts.tv_nsec/1000 + d->data->ts.tv_sec*1000000;
	clock_gettime(CLOCK_MONOTONIC, &d->data->ts);
	long new_us= d->data->ts.tv_nsec/1000 + d->data->ts.tv_sec*1000000;
	d->dt= (new_us - old_us)/1000000.0;
}

void plat_sleep(int ms)
{
	usleep(ms*1000);
}

