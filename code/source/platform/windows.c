#include "core/vector.h"
#include "platform/device.h"
#include "platform/dll.h"
#include "global/env.h"

#include <windows.h>
#include <tchar.h>

typedef struct DevicePlatformData {
	HDC hDC;
	HWND hWnd;
	HGLRC hGlrc;
	DWORD ticks;
	bool closeMessage;
	bool lbuttondownMessage;
} DevicePlatformData;

VoidFunc plat_query_gl_func_impl(const char *name)
{ return (VoidFunc)wglGetProcAddress(name); }

internal
LRESULT CALLBACK wndproc(
	HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		case WM_CLOSE:
			g_env.device->impl->closeMessage= true;
		break;
		case WM_MOUSEMOVE:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
		case WM_LBUTTONDOWN: 
			g_env.device->impl->lbuttondownMessage= true;
		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void plat_init_impl(Device* d, const char* title, V2i reso)
{
	d->impl= zero_malloc(sizeof(*d->impl));

	WNDCLASS wc= {}; 
	wc.lpfnWndProc= wndproc;
	wc.hInstance= GetModuleHandle(0);
	wc.hbrBackground= (HBRUSH)(COLOR_BACKGROUND);
	wc.lpszClassName= title;
	wc.style = CS_OWNDC;
	if( !RegisterClass(&wc) )
			fail("RegisterClass failed\n");
	d->impl->hWnd= CreateWindow(
		wc.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		0, 0, reso.x, reso.y, 0, 0, wc.hInstance, 0);

	// Create OpenGL context
	PIXELFORMATDESCRIPTOR pfd= {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32, // Framebuffer
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24, // Depth buffer
		8, // Stencil buffer
		0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};

	d->impl->hDC= GetDC(d->impl->hWnd);
	int choose= ChoosePixelFormat(d->impl->hDC, &pfd);
	SetPixelFormat(d->impl->hDC, choose, &pfd);
	d->impl->hGlrc= wglCreateContext(d->impl->hDC);
	wglMakeCurrent(d->impl->hDC, d->impl->hGlrc);
	
	d->impl->ticks= GetTickCount();
}

void plat_quit_impl(Device *d)
{
	fail("@todo quit");

	free(d->impl);
	d->impl= NULL;
	free(d);
}

void plat_update_impl(Device *d)
{
	fail("@todo update");
}

void plat_sleep_impl(int ms)
{
	fail("@todo sleep");
}

void plat_find_paths_with_end_impl(	char **path_table, U32 *path_count, U32 max_count,
					const char *name, int level, const char *end)
{
	char path[MAX_PATH_SIZE];
	fmt_str(path, sizeof(path), "%s*", name);
	if (strlen(path) >= MAX_PATH_SIZE - 1)
		fail("Too long path: %s", name);

	WIN32_FIND_DATA ffd;
	HANDLE h= FindFirstFile(path, &ffd);
	if (h == INVALID_HANDLE_VALUE) {
		fail("plat_find_paths_with_end_impl failed with %s: %i", path, GetLastError());
	}
	ensure(h != INVALID_HANDLE_VALUE);
	while (FindNextFile(h, &ffd)) {
		if (ffd.cFileName[0] == '.')
			continue;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			fmt_str(path, sizeof(path), "%s%s/", name, ffd.cFileName);
			plat_find_paths_with_end_impl(path_table, path_count, max_count, path, level + 1, end);
		} else {
			if (is_str_end(ffd.cFileName, end)) {
				if (*path_count + 1 >= max_count)
					fail(	"plat_find_paths_with_end: "
							"too many found files (@todo Fix engine)");
				U32 path_size= strlen(name) + 1 + strlen(ffd.cFileName) + 1;
				char *path= malloc(path_size);
				if (name[strlen(name) - 1] == '/')
					fmt_str(path, path_size, "%s%s", name, ffd.cFileName);
				else
					fmt_str(path, path_size, "%s/%s", name, ffd.cFileName);
				path_table[*path_count]= path;
				++*path_count;
			}
		}
	}
	FindClose(h);
}

DllHandle load_dll(const char *path)
{ return LoadLibrary(path); }

void unload_dll(DllHandle dll)
{ FreeLibrary((HMODULE)dll); }

void* query_dll_sym(DllHandle dll, const char *sym)
{ return (void*)GetProcAddress((HMODULE)dll, sym); }

const char * dll_error()
{ return "@todo dllError on windows"; }

const char * plat_dll_ext()
{ return "dll"; }

void plat_set_term_color(TermColor c)
{
	int i;
	switch (c) {
	case TermColor_default:
		i=	FOREGROUND_RED |
			FOREGROUND_GREEN |
			FOREGROUND_BLUE;
	break;
	case TermColor_red: i= FOREGROUND_RED; break;
	default: fail("plat_set_term_color: Unknown color: %i", c);
	}

	HANDLE h= GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(h, i);
}

int fmt_str(char *str, U32 size, const char *fmt, ...)
{
	if (size == 0)
		return 0;

	va_list args;
	va_start(args, fmt);
	// For some reason this mingw distro doesn't have vsnprintf_s. Must terminate manually
	int ret= vsnprintf(str, size, fmt, args);
	if (ret >= size) {
		str[size - 1]= 0;
		ret= size - 1;
	}
	va_end(args);
	return ret;
}

