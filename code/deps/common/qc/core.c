#include "core.h"

QC_DEFINE_ARRAY(char)
QC_DEFINE_ARRAY(int)

void *qc_nonull_impl(void *ptr)
{
	if (!ptr)
		abort();
	return ptr;
}

QC_Bool qc_buf_str_equals(QC_Buf_Str a, QC_Buf_Str b)
{
	if (a.len != b.len)
		return QC_false;
	return !strncmp(a.buf, b.buf, a.len);
}

QC_Buf_Str qc_c_str_to_buf_str(const char* str)
{
	QC_Buf_Str b = {0};
	b.buf = str;
	b.len = strlen(str);
	return b;
}

void qc_safe_vsprintf(QC_Array(char) *buf, const char *fmt, va_list args)
{
	char tmp[1024*100]; /* :( */
	int len;
	int i;

	/* @todo Find open source non-gpl snprintf */
	len = vsprintf(tmp, fmt, args);
	if (len < 0)
		return;
	if (len > (int)sizeof(tmp)/2) /* Crappy failsafe */
		abort();

	if (buf->size > 0 && buf->data[buf->size - 1] == '\0')
		qc_pop_array(char)(buf);

	for (i = 0; i < len; ++i)
		qc_push_array(char)(buf, tmp[i]);
	qc_push_array(char)(buf, '\0');
}

void qc_append_str(QC_Array(char) *buf, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	qc_safe_vsprintf(buf, fmt, args);
	va_end(args);
}


