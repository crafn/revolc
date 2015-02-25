#ifndef TARGET_H
#define TARGET_H

extern struct symbol *size_t_ctype;
extern struct symbol *ssize_t_ctype;

/*
 * For "__attribute__((aligned))"
 */
extern int max_alignment;

/*
 * Integer data types
 */
extern int bits_in_bool;
extern int bits_in_char;
extern int bits_in_short;
extern int bits_in_int;
extern int bits_in_long;
extern int bits_in_longlong;
extern int bits_in_longlonglong;

extern int max_int_alignment;

/*
 * Floating point data types
 */
extern int bits_in_float;
extern int bits_in_double;
extern int bits_in_longdouble;

extern int max_fp_alignment;

/*
 * Pointer data type
 */
extern int bits_in_pointer;
extern int pointer_alignment;

/*
 * Enum data types
 */
extern int bits_in_enum;
extern int enum_alignment;

/*
 * Helper functions for converting bits to bytes and vice versa.
 */

static inline int bits_to_bytes(int bits)
{
	return bits >= 0 ? bits / bits_in_char : -1;
}

static inline int bytes_to_bits(int bytes)
{
	return bytes * bits_in_char;
}

#endif
