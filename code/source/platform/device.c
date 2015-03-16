#include "device.h"
#include "gl.h"
#include "core/malloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

/// @todo Think how debug_print and plat_print relate
#define plat_print printf

internal
void plat_fail(const char *msg)
{
	printf("%s\n", msg);
	abort();
}

typedef void (*VoidFunc)();
internal
VoidFunc plat_query_gl_func(const char *name)
{
	VoidFunc f= NULL;
#if PLATFORM == PLATFORM_LINUX
	f= glXGetProcAddressARB((const GLubyte*)name);
#elif PLATFORM == PLATFORM_WINDOWS
	f= (voidFunc)wglGetProcAddress(name);
#endif
	if (!f) {
		plat_print("Failed to query gl function: %s\n", name);
		plat_fail("");
	}
	return f;
}

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

Device * plat_init(const char* title, int width, int height)
{
	debug_print("plat_init");
	Device *d= zero_malloc(sizeof(*d));
	if (g_env.device == NULL)
		g_env.device= d;
	d->data= zero_malloc(sizeof(*d->data));
	memset(d->data, 0, sizeof(*d->data));

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
					  0, 0, width, height, 0, vi->depth, InputOutput, 
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

		if (!glXCreateContextAttribsARB)
			plat_fail( "glXCreateContextAttribsARB() not found");

		// If it does, try to get a GL 3.2 context!
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

		d->data->dpy= display;
		d->data->win= win;
		d->data->ctx= ctx;
	}

	{
		clock_gettime(CLOCK_MONOTONIC, &d->data->ts);
	}

	{
		glCreateShader= (GlCreateShader)plat_query_gl_func("glCreateShader");
		glShaderSource= (GlShaderSource)plat_query_gl_func("glShaderSource");
		glCompileShader= (GlCompileShader)plat_query_gl_func("glCompileShader");
		glCreateProgram= (GlCreateProgram)plat_query_gl_func("glCreateProgram");
		glAttachShader= (GlAttachShader)plat_query_gl_func("glAttachShader");
		glLinkProgram= (GlLinkProgram)plat_query_gl_func("glLinkProgram");
		glUseProgram= (GlUseProgram)plat_query_gl_func("glUseProgram");
		glGetShaderiv= (GlGetShaderiv)plat_query_gl_func("glGetShaderiv");
		glGetProgramiv= (GlGetProgramiv)plat_query_gl_func("glGetProgramiv");
		glGetShaderInfoLog= (GlGetShaderInfoLog)plat_query_gl_func("glGetShaderInfoLog");
		glGetProgramInfoLog= (GlGetProgramInfoLog)plat_query_gl_func("glGetProgramInfoLog");
		glDetachShader= (GlDetachShader)plat_query_gl_func("glDetachShader");
		glDeleteShader= (GlDeleteShader)plat_query_gl_func("glDeleteShader");
		glDeleteProgram= (GlDeleteProgram)plat_query_gl_func("glDeleteProgram");
		glGetUniformLocation= (GlGetUniformLocation)plat_query_gl_func("glGetUniformLocation");
		glUniform1f= (GlUniform1f)plat_query_gl_func("glUniform1f");
		glUniform2f= (GlUniform2f)plat_query_gl_func("glUniform2f");
		glUniform3f= (GlUniform3f)plat_query_gl_func("glUniform3f");
		glUniform4f= (GlUniform4f)plat_query_gl_func("glUniform4f");
		glUniformMatrix4fv= (GlUniformMatrix4fv)plat_query_gl_func("glUniformMatrix4fv");
		glUniform1i= (GlUniform1i)plat_query_gl_func("glUniform1i");
		glGenBuffers= (GlGenBuffers)plat_query_gl_func("glGenBuffers");
		glBindBuffer= (GlBindBuffer)plat_query_gl_func("glBindBuffer");
		glBufferData= (GlBufferData)plat_query_gl_func("glBufferData");
		glBufferSubData= (GlBufferSubData)plat_query_gl_func("glBufferSubData");
		glDeleteBuffers= (GlDeleteBuffers)plat_query_gl_func("glDeleteBuffers");
		glEnableVertexAttribArray= (GlEnableVertexAttribArray)plat_query_gl_func("glEnableVertexAttribArray");
		glDisableVertexAttribArray= (GlDisableVertexAttribArray)plat_query_gl_func("glDisableVertexAttribArray");
		glVertexAttribPointer= (GlVertexAttribPointer)plat_query_gl_func("glVertexAttribPointer");
		glBindAttribLocation= (GlBindAttribLocation)plat_query_gl_func("glBindAttribLocation");

		glGenFramebuffers= (GlGenFramebuffers)plat_query_gl_func("glGenFramebuffers");
		glBindFramebuffer= (GlBindFramebuffer)plat_query_gl_func("glBindFramebuffer");
		glFramebufferTexture2D= (GlFramebufferTexture2D)plat_query_gl_func("glFramebufferTexture2D");
		glDeleteFramebuffers= (GlDeleteFramebuffers)plat_query_gl_func("glDeleteFramebuffers");
		glGenVertexArrays= (GlGenVertexArrays)plat_query_gl_func("glGenVertexArrays");
		glDeleteVertexArrays= (GlDeleteVertexArrays)plat_query_gl_func("glDeleteVertexArrays");
		glBindVertexArray= (GlBindVertexArray)plat_query_gl_func("glBindVertexArray");
		glTexStorage3D= (GlTexStorage3D)plat_query_gl_func("glTexStorage3D");
		//glTexSubImage3D= (GlTexSubImage3D)plat_query_gl_func("glTexSubImage3D");
	}

	return d;
}

void plat_quit(Device *d)
{
	if (g_env.device == d)
		g_env.device= NULL;

	glXMakeCurrent(d->data->dpy, None, NULL);
	glXDestroyContext(d->data->dpy, d->data->ctx);
	XDestroyWindow(d->data->dpy, d->data->win);
	XCloseDisplay(d->data->dpy);

	free(d->data);
	d->data= NULL;
	free(d);
	
	debug_print("plat_quit successful");
}

void plat_update(Device *d)
{
	glXSwapBuffers(d->data->dpy, d->data->win);

	for (int i= 0; i < KEYBOARD_KEY_COUNT; ++i)
		d->key_pressed[i]= d->key_released[i]= false;

	while(XPending(d->data->dpy)) {
		XEvent xev;
        XNextEvent(d->data->dpy, &xev);

		if (	xev.type == KeyPress ||
				xev.type == KeyRelease) {
			int keys_ret;
			KeySym* keysym=
				XGetKeyboardMapping(d->data->dpy, xev.xkey.keycode, 1, &keys_ret);

			if(xev.type == KeyPress) {
				if (*keysym == XK_Escape)
					d->quit_requested= true;
			}
			//debug_print("keysym: %i", *keysym);

			int table_index= 0;
			if (*keysym < KEYBOARD_KEY_COUNT)
				table_index= *keysym;
			else if (*keysym >= 65470 && *keysym <= 65481)
				table_index= KEY_F1 + (*keysym - 65470);
			else if (*keysym == 65289)
				table_index= KEY_TAB;
			else if (*keysym == 65505)
				table_index= KEY_LSHIFT;

			d->key_down[table_index]= (xev.type == KeyPress);
			d->key_pressed[table_index]= (xev.type == KeyPress);
			d->key_released[table_index]= (xev.type == KeyRelease);
			
			XFree(keysym);
		}

		if (xev.xbutton.type == ButtonPress || xev.xbutton.type == ButtonRelease) {
			int key= KEY_LMB;
			if (xev.xkey.keycode == 3)
				key= KEY_RMB;

			d->key_pressed[key]= (xev.xbutton.type == ButtonPress);
			d->key_down[key]= (xev.xbutton.type == ButtonPress);
			d->key_released[key]= (xev.xbutton.type == ButtonRelease);
		}
	}

	XWindowAttributes gwa;
	XGetWindowAttributes(d->data->dpy, d->data->win, &gwa);
	d->win_size[0]= gwa.width;
	d->win_size[1]= gwa.height;

	int root_x= 0, root_y= 0;
	Window w;
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


#define PATH_MAX_TABLE_SIZE 1024

// Thanks lloydm: http://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux 
internal
void listdir(char **path_table, U32 *path_count, const char *name, int level, const char *end)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir= opendir(name)))
		plat_fail("opendir failed");
    if (!(entry= readdir(dir)))
		plat_fail("readdir failed");

    do {
        if (entry->d_type == DT_DIR) {
            char path[1024]; /// @todo Dynamic allocation
            int len= snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			if (len + 1 >= 1024)
				plat_fail(	"plat_find_paths_with_end: "
							"Too long path (@todo Fix engine)");
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            listdir(path_table, path_count, path, level + 1, end);
        } else {
			if (is_str_end(entry->d_name, end)) {
				if (*path_count + 1 >= PATH_MAX_TABLE_SIZE)
					plat_fail(	"plat_find_paths_with_end: "
								"too many found files (@todo Fix engine)");
				U32 path_size= strlen(name) + 1 + strlen(entry->d_name) + 1;
				char *path= malloc(path_size);
				if (name[strlen(name) - 1] == '/')
					snprintf(path, path_size, "%s%s", name, entry->d_name);
				else
					snprintf(path, path_size, "%s/%s", name, entry->d_name);
				path_table[*path_count]= path;
				++*path_count;
			}
		}
    } while ((entry= readdir(dir)));

    closedir(dir);
}

char ** plat_find_paths_with_end(const char *path_to_dir, const char *end)
{
	U32 path_count= 0;
	char **path_table= zero_malloc(sizeof(*path_table)*PATH_MAX_TABLE_SIZE);

	listdir(path_table, &path_count, path_to_dir, 0, end);
	return path_table;
}

