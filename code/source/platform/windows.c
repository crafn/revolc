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
	U64 ticks; // Ticks from QueryPerformanceCounter -- not ms
	U64 timer_reso;
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
	case WM_MOUSEWHEEL:
		g_env.device->mwheel_delta=
			(F64)GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void plat_init_impl(Device* d, const char* title, V2i reso)
{
	d->win_size= reso;
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

	U64 ticks;
	if(!QueryPerformanceFrequency((LARGE_INTEGER *)&ticks))
		fail("QueryPerformanceFrequency failed!");
	d->impl->timer_reso= ticks;

	QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
	d->impl->ticks= ticks;
}

void plat_quit_impl(Device *d)
{
	wglDeleteContext(d->impl->hGlrc);

	free(d->impl);
	d->impl= NULL;
	free(d);
}

void plat_update_impl(Device *d)
{
	SwapBuffers(d->impl->hDC);

	d->mwheel_delta= 0.0;

	MSG msg;
	while(PeekMessage(&msg, d->impl->hWnd, 0, 0, PM_REMOVE) > 0) {
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	}

	const U32 keycode_to_vkcode[KEY_COUNT]= {
		VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, 0,
		0, 0, 0, 0,
		0, VK_TAB, VK_LSHIFT, VK_LCONTROL,
		VK_SPACE, VK_DELETE, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		// 64

		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		0, 0x41, 0x42, 0x43,
		0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4A, 0x4B,
		0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53,
		0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5A, 0,
		0, 0, 0, 0,

		// 128

		VK_F1, VK_F2, VK_F3, VK_F4,
		VK_F5, VK_F6, VK_F7, VK_F8,
		VK_F9, VK_F10, VK_F11, VK_F12,
		VK_ESCAPE, 0, 0, 0,
		VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		// 192

		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,

		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};
	bool has_focus= GetFocus() == d->impl->hWnd;
	for (U32 i= 0; i < KEY_COUNT; ++i) {
		int vk= keycode_to_vkcode[i];
		if (vk == 0)
			continue;
		bool was_down= d->key_down[i];
		d->key_down[i]= (GetKeyState(vk) & 0x8000) && has_focus;
		d->key_pressed[i]= !was_down && d->key_down[i];
		d->key_released[i]= was_down && !d->key_down[i];
	}

	if (g_env.device->impl->closeMessage)
		d->quit_requested= true;

	RECT rect;
	if(GetClientRect(d->impl->hWnd, &rect)) {
		d->win_size.x= rect.right - rect.left;
		d->win_size.y= rect.bottom - rect.top;
	}
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(d->impl->hWnd, &cursor);
	d->cursor_pos.x= cursor.x;
	d->cursor_pos.y= cursor.y;


	const U64 old_ticks= d->impl->ticks;
	U64 new_ticks;
	QueryPerformanceCounter((LARGE_INTEGER *)&new_ticks);

	d->dt= (new_ticks - old_ticks)*1.0/d->impl->timer_reso;
	d->impl->ticks= new_ticks;
}

void plat_sleep_impl(int ms)
{
	Sleep(ms);
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

int v_fmt_str(char *str, U32 size, const char *fmt, va_list args)
{
	if (size == 0 && str)
		return 0;

	// For some reason this mingw distro doesn't have vsnprintf_s. Must terminate manually
	int ret= vsnprintf(str, size, fmt, args);
	if (str && ret >= size) {
		str[size - 1]= 0;
		ret= size - 1;
	}
	return ret;
}

