#ifndef REVOLC_PLATFORM_TERM_H
#define REVOLC_PLATFORM_TERM_H

#include "build.h"

typedef enum {
	TermColor_default,
	TermColor_red,
} TermColor;

void plat_set_term_color(TermColor c);

#endif // REVOLC_PLATFORM_TERM_H
