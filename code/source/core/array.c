#include "array.h"
#include "ensure.h"
#include "platform/stdlib.h"

void* enlarge_array(void *array, U32 *old_c, U32 elem_size)
{
	U32 old_size= *old_c*elem_size;
	U32 new_c= *old_c*1.5;
	U32 new_size= new_c*elem_size;
	ensure(new_c > *old_c);

	array= realloc(array, new_size);
	memset(array + old_size, 0, new_size - old_size);
	*old_c= new_c;
	return array;
}

void* push_dyn_array(
		void *array, U32 *capacity, U32 *count, U32 elem_size, void *elem)
{
	if (*capacity == *count)
		array= enlarge_array(array, capacity, elem_size);
	memcpy(array + (*count)*elem_size, elem, elem_size);
	++(*count);
	return array;
}
