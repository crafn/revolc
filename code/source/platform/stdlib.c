#include "stdlib.h"

int fmt_str(char *str, U32 size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret= v_fmt_str(str, size, fmt, args);
	va_end(args);
	return ret;
}
