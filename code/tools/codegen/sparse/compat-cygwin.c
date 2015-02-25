/*	
 * Cygwin Compatibility functions	
 *	
 *	
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */	
	
	
	
#include <sys/mman.h>	
#include <stdlib.h>	
#include <string.h>	
#include <sys/stat.h>

#include "lib.h"
#include "allocate.h"
#include "token.h"
	
void *blob_alloc(unsigned long size)	
{	
	void *ptr;	
	size = (size + 4095) & ~4095;	
	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);	
	if (ptr == MAP_FAILED)	
		ptr = NULL;	
	else	
		memset(ptr, 0, size);	
	return ptr;	
}	
	
void blob_free(void *addr, unsigned long size)	
{	
	size = (size + 4095) & ~4095;	
	munmap(addr, size);	
}	
	
long double string_to_ld(const char *nptr, char **endptr) 	
{	
	return strtod(nptr, endptr);	
}
