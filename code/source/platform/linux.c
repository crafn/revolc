#include "core/vector.h"
#include "platform/dll.h"

#include <dlfcn.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct DevicePlatformData {
	Display* dpy;
	Window win;
	GLXContext ctx;
	struct timespec ts;
} DevicePlatformData;

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

internal
Bool ctxErrorOccurred = false;
internal
int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
	ctxErrorOccurred = true;
	return 0;
}

#define plat_print debug_print
#define plat_fail fail

VoidFunc plat_query_gl_func_impl(const char *name)
{ return glXGetProcAddressARB((const GLubyte*)name); }

void plat_init_impl(Device* d, const char* title, V2i reso)
{
	d->impl= zero_malloc(sizeof(*d->impl));
	{
		/// Original code from https://www.opengl.org/wiki/Tutorial:_OpenGL_3.0_Context_Creation_%28GLX%29
		Display *display = XOpenDisplay(NULL);

		if (!display)
		{
			plat_print("Failed to open X display\n");
			exit(1);
		}

		// Get a matching FB config
		static int visual_attribs[] =
		{
			GLX_X_RENDERABLE    , True,
			GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
			GLX_RENDER_TYPE     , GLX_RGBA_BIT,
			GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
			GLX_RED_SIZE        , 8,
			GLX_GREEN_SIZE      , 8,
			GLX_BLUE_SIZE       , 8,
			GLX_ALPHA_SIZE      , 8,
			GLX_DEPTH_SIZE      , 24,
			GLX_STENCIL_SIZE    , 8,
			GLX_DOUBLEBUFFER    , True,
			//GLX_SAMPLE_BUFFERS  , 1,
			//GLX_SAMPLES         , 4,
			None
		};

		int fbcount;
		GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
		if (!fbc)
		{
			plat_print( "Failed to retrieve a framebuffer config\n" );
			exit(1);
		}

		// Pick the FB config/visual with the lowest samples per pixel
		int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

		int i;
		for (i=0; i<fbcount; ++i)
		{
			XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
			if ( vi )
			{
				int samp_buf, samples;
				glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
				glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );

				if ( best_fbc < 0 || (samp_buf && samples < best_num_samp) )
					best_fbc = i, best_num_samp = samples;
				if ( worst_fbc < 0 || !samp_buf || samples > worst_num_samp )
					worst_fbc = i, worst_num_samp = samples;
			}
			XFree( vi );
		}

		GLXFBConfig bestFbc = fbc[ best_fbc ];

		// Be sure to free the FBConfig list allocated by glXChooseFBConfig()
		XFree( fbc );

		// Get a visual
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );

		XSetWindowAttributes swa;
		Colormap cmap;
		swa.colormap = cmap = XCreateColormap( display,
								 RootWindow( display, vi->screen ), 
								 vi->visual, AllocNone );
		swa.background_pixmap = None ;
		swa.border_pixel      = 0;
		swa.event_mask= ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;

		Window win = XCreateWindow( display, RootWindow( display, vi->screen ), 
					  0, 0, reso.x, reso.y, 0, vi->depth, InputOutput, 
					  vi->visual, 
					  CWBorderPixel|CWColormap|CWEventMask, &swa );
		if ( !win )
		{
			plat_print( "Failed to create window.\n" );
			exit(1);
		}

		// Done with the visual info data
		XFree( vi );

		XStoreName( display, win, title );

		//plat_print( "Mapping window\n" );
		XMapWindow( display, win );

		// NOTE: It is not necessary to create or make current to a context before
		// calling glXGetProcAddressARB
		glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
		glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
		glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

		GLXContext ctx = 0;

		// Install an X error handler so the application won't exit if GL 3.0
		// context allocation fails.
		//
		// Note this error handler is global.  All display connections in all threads
		// of a process use the same error handler, so be sure to guard against other
		// threads issuing X commands while this code is running.
		ctxErrorOccurred = false;
		int (*oldHandler)(Display*, XErrorEvent*) =
		XSetErrorHandler(&ctxErrorHandler);

		if (!glXCreateContextAttribsARB) {
			plat_fail( "glXCreateContextAttribsARB() not found");

		// If it does, try to get a GL 3.2 context!
		}
		else
		{
			int context_attribs[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
				GLX_CONTEXT_MINOR_VERSION_ARB, 2,
				//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				None
			};

			ctx = glXCreateContextAttribsARB( display, bestFbc, 0,
								  True, context_attribs );

			// Sync to ensure any errors generated are processed->
			XSync( display, False );
			if ( !ctxErrorOccurred && ctx )
				plat_print("Created GL 3.2 context\n");
			else
				plat_fail("Couldn't create GL 3.2 context");
		}
		// Sync to ensure any errors generated are processed->
		XSync( display, False );

		// Restore the original error handler
		XSetErrorHandler( oldHandler );

		if ( ctxErrorOccurred || !ctx )
		{
			plat_print( "Failed to create an OpenGL context\n" );
			exit(1);
		}

		// Verifying that context is a direct context
		if ( ! glXIsDirect ( display, ctx ) )
		{
			plat_print( "Indirect GLX rendering context obtained\n" );
		}

		glXMakeCurrent(display, win, ctx);

		d->impl->dpy= display;
		d->impl->win= win;
		d->impl->ctx= ctx;
	}

	{
		clock_gettime(CLOCK_MONOTONIC, &d->impl->ts);
	}
}

void plat_quit_impl(Device *d)
{
	glXMakeCurrent(d->impl->dpy, None, NULL);
	glXDestroyContext(d->impl->dpy, d->impl->ctx);
	XDestroyWindow(d->impl->dpy, d->impl->win);
	XCloseDisplay(d->impl->dpy);

	free(d->impl);
	d->impl= NULL;
	free(d);
}

void plat_update_impl(Device *d)
{
	glXSwapBuffers(d->impl->dpy, d->impl->win);

	d->mwheel_delta= 0.0;
	for (int i= 0; i < KEY_COUNT; ++i)
		d->key_pressed[i]= d->key_released[i]= false;
	d->quit_requested= false;

	while(XPending(d->impl->dpy)) {
		XEvent xev;
        XNextEvent(d->impl->dpy, &xev);

		if (	xev.type == KeyPress ||
				xev.type == KeyRelease) {
			int keys_ret;
			KeySym* keysym=
				XGetKeyboardMapping(d->impl->dpy, xev.xkey.keycode, 1, &keys_ret);

			if(xev.type == KeyPress) {
				if (*keysym == XK_Escape)
					d->quit_requested= true;
			}
			//debug_print("keysym: %i", *keysym);

			int table_index= 0;
			if (*keysym < KEY_COUNT)
				table_index= *keysym;
			else if (*keysym >= 65470 && *keysym <= 65481)
				table_index= KEY_F1 + (*keysym - 65470);
			else if (*keysym == 65289)
				table_index= KEY_TAB;
			else if (*keysym == 65505)
				table_index= KEY_LSHIFT;
			else if (*keysym == 65307)
				table_index= KEY_ESC;
			else if (*keysym == 65361)
				table_index= KEY_LEFT;
			else if (*keysym == 65362)
				table_index= KEY_UP;
			else if (*keysym == 65363)
				table_index= KEY_RIGHT;
			else if (*keysym == 65364)
				table_index= KEY_DOWN;

			d->key_down[table_index]= (xev.type == KeyPress);
			d->key_pressed[table_index]= (xev.type == KeyPress);
			d->key_released[table_index]= (xev.type == KeyRelease);
			
			XFree(keysym);
		}

		// Mouse buttons
		if (	xev.xkey.keycode >= 1 && xev.xkey.keycode <= 3 &&
				(xev.xbutton.type == ButtonPress ||
				 xev.xbutton.type == ButtonRelease)) {
			int key= KEY_LMB;
			if (xev.xkey.keycode == 2)
				key= KEY_MMB;
			if (xev.xkey.keycode == 3)
				key= KEY_RMB;

			d->key_pressed[key]= (xev.xbutton.type == ButtonPress);
			d->key_down[key]= (xev.xbutton.type == ButtonPress);
			d->key_released[key]= (xev.xbutton.type == ButtonRelease);
		}

		// Scroll
		if (xev.xbutton.type == ButtonPress && xev.xkey.keycode == 4)
			d->mwheel_delta= 1.0;
		if (xev.xbutton.type == ButtonPress && xev.xkey.keycode == 5)
			d->mwheel_delta= -1.0;
	}

	XWindowAttributes gwa;
	XGetWindowAttributes(d->impl->dpy, d->impl->win, &gwa);
	d->win_size.x= gwa.width;
	d->win_size.y= gwa.height;

	int root_x= 0, root_y= 0;
	Window w;
	unsigned int mask;
	XQueryPointer(	d->impl->dpy, d->impl->win, &w,
					&w, &root_x, &root_y, &d->cursor_pos.x, &d->cursor_pos.y,
					&mask);

	long old_us= d->impl->ts.tv_nsec/1000 + d->impl->ts.tv_sec*1000000;
	clock_gettime(CLOCK_MONOTONIC, &d->impl->ts);
	long new_us= d->impl->ts.tv_nsec/1000 + d->impl->ts.tv_sec*1000000;
	d->dt= (new_us - old_us)/1000000.0;
}

void plat_sleep_impl(int ms)
{
	usleep(ms*1000);
}

// Thanks lloydm: http://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux 
void plat_find_paths_with_end_impl(char **path_table, U32 *path_count, U32 max_count, const char *name, int level, const char *end)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir= opendir(name)))
		plat_fail("opendir failed");
    if (!(entry= readdir(dir)))
		plat_fail("readdir failed");

    do {
        if (entry->d_type == DT_DIR) {
            char path[MAX_PATH_SIZE];
            int len= fmt_str(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			if (len + 1 >= 1024)
				plat_fail(	"plat_find_paths_with_end: "
							"Too long path (@todo Fix engine)");
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            plat_find_paths_with_end_impl(path_table, path_count, max_count, path, level + 1, end);
        } else {
			if (is_str_end(entry->d_name, end)) {
				if (*path_count + 1 >= max_count)
					plat_fail(	"plat_find_paths_with_end: "
								"too many found files (@todo Fix engine)");
				U32 path_size= strlen(name) + 1 + strlen(entry->d_name) + 1;
				char *path= malloc(path_size);
				if (name[strlen(name) - 1] == '/')
					fmt_str(path, path_size, "%s%s", name, entry->d_name);
				else
					fmt_str(path, path_size, "%s/%s", name, entry->d_name);
				path_table[*path_count]= path;
				++*path_count;
			}
		}
    } while ((entry= readdir(dir)));

    closedir(dir);
}

DllHandle load_dll(const char *path)
{
	if (dlopen(path, RTLD_NOLOAD))
		fail("DLL already opened: %s", path);
	return dlopen(path, RTLD_NOW);
}

void unload_dll(DllHandle dll)
{ dlclose(dll); }

void* query_dll_sym(DllHandle dll, const char *sym)
{ return dlsym(dll, sym); }

const char * dll_error()
{ return dlerror(); }

const char * plat_dll_ext()
{ return "so"; }

void plat_set_term_color(TermColor c)
{
	const char *str;
	switch (c) {
	case TermColor_default: str= "\033[0m"; break;
	case TermColor_red: str= "\033[0;31m"; break;
	default: fail("plat_set_term_color: Unknown color: %i", c);
	}
	printf("%s", str);
}

int fmt_str(char *str, U32 size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret= vsnprintf(str, size, fmt, args);
	va_end(args);
	return ret;
}
