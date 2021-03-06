#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sparse/lib.h>
#include <sparse/allocate.h>
#include <sparse/token.h>
#include <sparse/parse.h>
#include <sparse/scope.h>
#include <sparse/symbol.h>
#include <sparse/expression.h>
#include <sparse/linearize.h>

#define MAX_STRUCT_COUNT (1024*4)
#define MAX_STRUCT_MEMBER_COUNT 128

typedef struct StructInfo {
	struct symbol *sym;
	struct symbol *member_syms[MAX_STRUCT_MEMBER_COUNT];
	int member_count;
} StructInfo;

StructInfo struct_infos[MAX_STRUCT_COUNT];
int struct_info_count;

static void examine_namespace(struct symbol *sym)
{
	if (sym->ident && sym->ident->reserved)
		return;

	switch(sym->namespace) {
	case NS_MACRO:
		break;
	case NS_STRUCT: {
		if (sym->type == SYM_UNION)
			break;
		if (!sym->ident)
			break;
		if (!sym->ident->name)
			break;
		if (sym->ident->name[0] == '_')
			break;
		if (sym->ident->name[0] > 'Z')
			break;
		if (	sym->ident->name[0] == 'L' &&
				sym->ident->name[1] == 'o' &&
				sym->ident->name[2] == 'd' &&
				sym->ident->name[3] == 'e') // Lode png lib
			break;
		if (	sym->ident->name[0] == 'P' &&
				sym->ident->name[1] == 'a') // PortAudio
			break;

		{
			int illegal= 0;
			int count= 0;
			struct symbol *m= NULL;
			FOR_EACH_PTR(sym->symbol_list, m) {
				++count;
				if (!m->ident || !m->ident->name || m->ident->name[0] == '_') {
					illegal= 1;
					break;
				}
			} END_FOR_EACH_PTR(m);
			if (illegal || count == 0)
				break;
		}

		assert(struct_info_count < MAX_STRUCT_COUNT);
		StructInfo *s= &struct_infos[struct_info_count++];
		s->sym= sym;

		struct symbol *m;
		FOR_EACH_PTR(sym->symbol_list, m) {
			assert(s->member_count < MAX_STRUCT_MEMBER_COUNT);
			s->member_syms[s->member_count++]= m;
		} END_FOR_EACH_PTR(m);
		break;
	} case NS_TYPEDEF:
	case NS_SYMBOL:
	case NS_NONE:
	case NS_LABEL:
	case NS_ITERATOR:
	case NS_UNDEF:
	case NS_PREPROCESSOR:
	case NS_KEYWORD:
		break;
	default:
		die("Unrecognised namespace type %d",sym->namespace);
	}
}

static inline void examine_symbol_list(const char *file, struct symbol_list *list)
{
	struct symbol *sym;

	if (!list)
		return;
	FOR_EACH_PTR(list, sym) {
			examine_namespace(sym);
	} END_FOR_EACH_PTR(sym);
}

#define CODEGEN_LINE "// This file is automatically generated (codegen)\n\n"
static void write_rtti(const char *src_file, const char *dst_file)
{
	/// @todo Don't hard-code
	char *sparse_argv[]= {
		"codegen",
		"-std=c99",
		"-I/usr/include/",
		"-I/usr/include/x86_64-linux-gnu/",
		"-I/usr/lib/gcc/x86_64-linux-gnu/4.8/include/",
		"-ID:/ohjelmat/mingw_win-builds64/x86_64-w64-mingw32/include/",
		"-ID:/ohjelmat/mingw_win-builds64/lib64/gcc/x86_64-w64-mingw32/4.8.2/include/",
		"-I./source/",
		"-I./deps/common/",
		"-I./deps/common/chipmunk/include/",
		"-I./deps/common/ogg/include/",
		"-I./deps/common/vorbis/include/",
		"-I./deps/win/",
		"-I./deps/win/portaudio-19/include/",
		"-DCODEGEN",
		(char*)src_file,
		NULL
	};
	const int sparse_argc= sizeof(sparse_argv)/sizeof(*sparse_argv);

	struct string_list *filelist = NULL;
	char *file;
	sparse_initialize(sparse_argc, sparse_argv, &filelist);
	FOR_EACH_PTR_NOTAG(filelist, file) {
		sparse_keep_tokens(file);
		examine_symbol_list(file, file_scope->symbols);
	} END_FOR_EACH_PTR_NOTAG(file);

	//
	// Code generation
	//

	FILE *c= fopen(dst_file, "wb");
	assert(c);

	fprintf(c, "%s", CODEGEN_LINE);
	fprintf(c, "\n#include \"platform/stdlib.h\" // offsetof\n\n");

	for (int i= 0; i < struct_info_count; ++i) {
		StructInfo *s= &struct_infos[i];

		// Struct size
		fprintf(c, "DLL_EXPORT const U32 %s_size= sizeof(%s);\n", s->sym->ident->name, s->sym->ident->name);

		// Member names
		fprintf(c, "DLL_EXPORT const char *%s_member_names[]= {\n", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			fprintf(c, "	\"%s\",\n", s->member_syms[k]->ident->name);
		}
		fprintf(c, "	0\n};\n");

		// Member type names
		fprintf(c, "DLL_EXPORT const char *%s_member_type_names[]= {\n", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			const struct symbol *type_sym= s->member_syms[k]->ctype.base_type;
			if (type_sym->ident) {
				fprintf(c, "	\"%s\",\n", type_sym->ident->name);
			} else {
				const char *type_str= "<NOIDENT>";
				if (type_sym == &float_ctype)
					type_str= "F32";
				else if (type_sym == &double_ctype)
					type_str= "F64";
				else if (type_sym == &ushort_ctype)
					type_str= "U16";
				else if (type_sym == &sshort_ctype)
					type_str= "S16";
				else if (type_sym == &uint_ctype)
					type_str= "U32";
				else if (type_sym == &sint_ctype)
					type_str= "S32";
				else if (type_sym == &ullong_ctype)
					type_str= "U64";
				else if (type_sym == &sllong_ctype)
					type_str= "S64";
				else if (type_sym == &bool_ctype)
					type_str= "bool";
				/// @todo Rest
				fprintf(c, "	\"%s\",\n", type_str);
			}
		}
		fprintf(c, "	0\n};\n");

		// Member sizes
		fprintf(c, "DLL_EXPORT const U32 %s_member_sizes[]= {", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			struct symbol *m= get_base_type(s->member_syms[k]);
			int byte_size= m->bit_size/8;
			if (byte_size == 0) // Happens with bools for some reason
				byte_size= 1;
			fprintf(c, "%i, ", byte_size);
		}
		fprintf(c, "};\n");

		// Member offsets
		fprintf(c, "DLL_EXPORT const U32 %s_member_offsets[]= {\n", s->sym->ident->name);
		for (int k= 0; k < s->member_count; ++k) {
			struct symbol *m= s->member_syms[k];
			fprintf(c, "	offsetof(%s, %s),\n",
					s->sym->ident->name,
					m->ident->name);
		}
		fprintf(c, "};\n");
	}
	fclose(c);
}

static void write_math()
{
	{
		FILE *f= fopen("./source/core/vector.h", "wb");
		assert(f);

		typedef struct VecType {
			const char *name;
			const char *lc_name;
			const char *comp_type_name;
			int comp_count;
			int round_to;
			int direct_cast_to;
		} VecType;

		VecType vecs[]= {
			{"V2d", "v2d", "F64", 2, 2, 1},
			{"V2f", "v2f", "F32", 2, 2, 0},
			{"V2i", "v2i", "S32", 2,-1,-1},
			{"V3d", "v3d", "F64", 3,-1, 4},
			{"V3f", "v3f", "F32", 3,-1, 3},
		};
		const int vec_count= sizeof(vecs)/sizeof(*vecs);

		const char *comp_names[4]= {"x", "y", "z", "w"};

		fprintf(f, "#ifndef REVOLC_CORE_VECTOR_H\n");
		fprintf(f, "#define REVOLC_CORE_VECTOR_H\n\n");
		fprintf(f, "%s", CODEGEN_LINE);
		fprintf(f, "#include \"platform/math.h\"\n\n");

		for (int i= 0; i < vec_count; ++i) {
			VecType v= vecs[i];
			fprintf(f, "typedef struct %s {\n", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "	%s %s;\n", v.comp_type_name, comp_names[k]);
			fprintf(f, "} %s;\n\n", v.name);
		}

		for (int i= 0; i < vec_count; ++i) {
			VecType v= vecs[i];

			fprintf(f, "static\n");
			fprintf(f, "bool equals_%s(%s a, %s b)\n", v.lc_name, v.name, v.name);
			fprintf(f, "{ return ");
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s == b.%s && ", comp_names[k], comp_names[k]);
			fprintf(f, "1; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s add_%s(%s a, %s b)\n", v.name, v.lc_name, v.name, v.name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s + b.%s, ", comp_names[k], comp_names[k]);
			fprintf(f, "}; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s sub_%s(%s a, %s b)\n", v.name, v.lc_name, v.name, v.name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s - b.%s, ", comp_names[k], comp_names[k]);
			fprintf(f, "}; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s mul_%s(%s a, %s b)\n", v.name, v.lc_name, v.name, v.name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s * b.%s, ", comp_names[k], comp_names[k]);
			fprintf(f, "}; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s neg_%s(%s v)\n", v.name, v.lc_name, v.name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "-v.%s, ", comp_names[k]); 
			fprintf(f, "}; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s scaled_%s(%s s, %s v)\n", v.name, v.lc_name, v.comp_type_name, v.name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "s*v.%s, ", comp_names[k]);
			fprintf(f, "}; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s dot_%s(%s a, %s b)\n", v.comp_type_name, v.lc_name, v.name, v.name);
			fprintf(f, "{ return ");
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s*b.%s + ", comp_names[k], comp_names[k]);
			fprintf(f, "0; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s length_sqr_%s(%s v)\n", v.comp_type_name, v.lc_name, v.name);
			fprintf(f, "{ return ");
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "v.%s*v.%s + ", comp_names[k], comp_names[k]);
			fprintf(f, "0; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "F64 length_%s(%s v)\n", v.lc_name, v.name);
			fprintf(f, "{ return sqrt(length_sqr_%s(v)); }\n\n", v.lc_name);

			fprintf(f, "static\n");
			fprintf(f, "%s dist_sqr_%s(%s a, %s b)\n", v.comp_type_name, v.lc_name, v.name, v.name);
			fprintf(f, "{ return ");
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "(a.%s - b.%s)*(a.%s - b.%s) + ", comp_names[k], comp_names[k], comp_names[k], comp_names[k]);
			fprintf(f, "0; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "F64 dist_%s(%s a, %s b)\n", v.lc_name, v.name, v.name);
			fprintf(f, "{ return sqrt(dist_sqr_%s(a, b)); }\n\n", v.lc_name);

			fprintf(f, "static\n");
			fprintf(f, "%s normalized_%s(%s v)\n", v.name, v.lc_name, v.name);
			fprintf(f, "{ return scaled_%s(1.0/length_%s(v), v); }\n\n", v.lc_name, v.lc_name);

			fprintf(f, "static\n");
			fprintf(f, "%s lerp_%s(%s a, %s b, %s t)\n", v.name, v.lc_name, v.name, v.name, v.comp_type_name);
			fprintf(f, "{ return (%s) {", v.name);
			for (int k= 0; k < v.comp_count; ++k)
				fprintf(f, "a.%s*(1 - t) + b.%s*t, ", comp_names[k], comp_names[k]);
			fprintf(f, "}; }\n\n");

			if (v.comp_count == 2) {
				fprintf(f, "static\n");
				fprintf(f, "%s rot_%s(F64 f, %s v)\n", v.name, v.lc_name, v.name);
				fprintf(f, "{ return (%s) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }\n\n", v.name);
			}

			if (v.comp_count == 3) {
				fprintf(f, "static\n");
				fprintf(f, "%s cross_%s(%s a, %s b)\n", v.name, v.lc_name, v.name, v.name);
				fprintf(f, "{ return (%s) {a.y*b.z - b.y*a.z, b.x*a.z - a.x*b.z, a.x*b.y - b.x*a.y}; }\n\n", v.name);
			}

			if (v.round_to != -1) {
				VecType r= vecs[v.round_to];
				fprintf(f, "static\n");
				fprintf(f, "%s round_%s_to_%s(%s v)\n", r.name, v.lc_name, r.lc_name, v.name);
				fprintf(f, "{ return (%s) {", r.name);
				for (int k= 0; k < v.comp_count; ++k)
					fprintf(f, "floor(v.%s + 0.5), ", comp_names[k]); 
				fprintf(f, "}; }\n\n");
			}

			if (v.direct_cast_to != -1) {
				VecType c= vecs[v.direct_cast_to];
				fprintf(f, "static\n");
				fprintf(f, "%s %s_to_%s(%s v)\n", c.name, v.lc_name, c.lc_name, v.name);
				fprintf(f, "{ return (%s) {", c.name);
				for (int k= 0; k < v.comp_count; ++k)
					fprintf(f, "v.%s, ", comp_names[k]); 
				fprintf(f, "}; }\n\n");
			}
		}

		fprintf(f,
			"static\n"
			"V2d v3d_to_v2d(V3d v)\n"
			"{ return (V2d) {v.x, v.y}; }\n\n");

		fprintf(f,
			"static\n"
			"V3d v2d_to_v3d(V2d v)\n"
			"{ return (V3d) {v.x, v.y, 0.0}; }\n");

		fprintf(f, "\n#endif\n");

		fclose(f);
	}

	{
		FILE *f= fopen("./source/core/quaternion.h", "wb");
		assert(f);

		typedef struct QuatInfo {
			const char *name;
			const char *lc_name;
			const char *comp_type_name;
			const char *axis_type_name;
			const char *axis_lc_name;
		} QuatInfo;

		QuatInfo quats[]= {
			{"Qf", "qf", "F32", "V3f", "v3f"},
			{"Qd", "qd", "F64", "V3d", "v3d"},
		};
		const int quat_count= sizeof(quats)/sizeof(*quats);

		const char *comp_names[4]= {"x", "y", "z", "w"};

		fprintf(f, "#ifndef REVOLC_CORE_QUATERNION_H\n");
		fprintf(f, "#define REVOLC_CORE_QUATERNION_H\n\n");
		fprintf(f, "%s", CODEGEN_LINE);

		fprintf(f, "#include \"math_constants.h\"\n");
		fprintf(f, "#include \"platform/stdlib.h\" // abs\n\n");
		fprintf(f, "#include \"vector.h\"\n\n");

		for (int i= 0; i < quat_count; ++i) {
			QuatInfo q= quats[i];

			fprintf(f, "typedef struct %s {\n", q.name);
			for (int k= 0; k < 4; ++k) {
				fprintf(f, "	%s %s;\n", q.comp_type_name, comp_names[k]);
			}
			fprintf(f, "} %s;\n\n", q.name);
		}
		for (int i= 0; i < quat_count; ++i) {
			QuatInfo q= quats[i];

			fprintf(f, "static\n");
			fprintf(f, "bool equals_%s(%s a, %s b)\n", q.lc_name, q.name, q.name);
			fprintf(f, "{ return ");
			for (int k= 0; k < 4; ++k)
				fprintf(f, "a.%s == b.%s && ", comp_names[k], comp_names[k]);
			fprintf(f, "1; }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s mul_%s(%s a, %s b)\n", q.name, q.lc_name, q.name, q.name);
			fprintf(f, "{ return (%s) {\n"
						"	a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,\n"
						"	a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,\n"
						"	a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,\n"
						"	a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z };\n}\n\n", q.name);

			fprintf(f, "static\n");
			fprintf(f, "%s dot_%s(%s a, %s b)\n", q.comp_type_name, q.lc_name, q.name, q.name);
			fprintf(f, "{ return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;\n}\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s neg_%s(%s q)\n", q.name, q.lc_name, q.name);
			fprintf(f, "{ return (%s) {-q.x, -q.y, -q.z, q.w}; }\n\n", q.name);

			fprintf(f, "static\n");
			fprintf(f, "%s rot_%s(%s q, %s v)\n", q.axis_type_name, q.axis_lc_name, q.name, q.axis_type_name);
			fprintf(f, "{\n"
						"	%s a, b, c= {q.x, q.y, q.z};\n"
						"	a= cross_%s(c, v);\n"
						"	b= cross_%s(c, a);\n"
						"	a= scaled_%s(2.0 * q.w, a);\n"
						"	b= scaled_%s(2.0, b);\n"
						"	return add_%s(add_%s(v, a), b);\n}\n\n",
						q.axis_type_name, q.axis_lc_name, q.axis_lc_name,
						q.axis_lc_name, q.axis_lc_name, q.axis_lc_name, q.axis_lc_name);

			fprintf(f, "static\n");
			fprintf(f, "%s rotation_z_%s(%s q)\n", q.comp_type_name, q.lc_name, q.name);
			fprintf(f, "{\n"
						"	/// @note Copy-pasted from old engine\n"
						"	/// @note	If fixing is needed, make sure that these still work:\n"
						"	///			 - physics object mirroring (e.g. stoneFlail in hand)\n"
						"	///			 - full rotation in 2d plane (e.g. grassClump attached to object)\n"
						"	bool flip= rot_%s(q, (%s) {0, 0, 1}).z < 0;\n"
						"	// Euler angles\n"
						"	F64 heading= atan2(2.0*q.y*q.w - 2.0*q.x*q.z, 1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z);\n"
						"	F64 attitude= asin(2.0*q.x*q.y + 2.0*q.z*q.w);\n"
						"	// Using heading to detect rotations greater than +-90 degrees\n"
						"	// Flipping was adjusted by trial & error\n"
						"	if (abs(heading) < PI*0.5 || flip)\n"
						"		return flip ? -attitude : attitude;\n"
						"	else\n"
						"		return PI - attitude;\n}\n\n",
						q.axis_lc_name, q.axis_type_name);

			fprintf(f, "static\n");
			fprintf(f, "%s identity_%s()\n", q.name, q.lc_name);
			fprintf(f, "{ return (%s) {0, 0, 0, 1}; }\n\n", q.name);

			fprintf(f, "static\n");
			fprintf(f, "%s normalized_%s(%s q)\n", q.name, q.lc_name, q.name);
			fprintf(f, "{\n"
						"	%s a = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;\n"
						"	if (a == 1)\n"
						"		return q;\n"
						"	else if (a == 0)\n"
						"		return identity_%s();\n"
						"	a= sqrt(a);\n"
						"	return (%s) {q.x/a, q.y/a, q.z/a, q.w/a};\n}\n\n",
						q.comp_type_name, q.lc_name, q.name);


			fprintf(f, "static\n");
			fprintf(f, "%s axis_%s(%s q)\n", q.axis_type_name, q.lc_name, q.name);
			fprintf(f, "{\n"
						"	%s s= sqrt(q.x*q.x + q.y*q.y + q.z*q.z);\n"
						"	if (s <= EPSILON || q.w > 1.0 || q.w < -1.0) {\n"
						"		return (%s) {0, 1, 0};\n"
						"	} else {\n"
						"		%s inv_s= 1.0/s;\n"
						"		return (%s) {q.x*inv_s, q.y*inv_s, q.z*inv_s};"
						"	}\n}\n\n",
						q.comp_type_name, q.axis_type_name, q.comp_type_name, q.axis_type_name);

			fprintf(f, "static\n");
			fprintf(f, "%s angle_%s(%s q)\n", q.comp_type_name, q.lc_name, q.name);
			fprintf(f, "{ return 2.0*acos(q.w); }\n\n");

			fprintf(f, "static\n");
			fprintf(f, "%s %s_by_axis(%s axis, %s angle)\n", q.name, q.lc_name, q.axis_type_name, q.comp_type_name);
			fprintf(f, "{\n"
						"	axis= normalized_%s(axis);\n"
						"	%s half= 0.5*angle;\n"
						"	%s s= sin(half);\n"
						"	return (%s) {axis.x*s, axis.y*s, axis.z*s, cos(half)};\n}\n\n",
						q.axis_lc_name, q.comp_type_name, q.comp_type_name, q.name);

			fprintf(f, "static\n");
			fprintf(f, "%s %s_by_from_to(%s v1, %s v2)\n", q.name, q.lc_name, q.axis_type_name, q.axis_type_name);
			fprintf(f, "{\n"
						"	v1= normalized_%s(v1); v2= normalized_%s(v2);\n"
						"	F64 dot= dot_%s(v1, v2);\n"
						"	if (dot >= 1.0) {\n"
						"		return identity_%s();\n"
						"	} else if (dot <= -1.0) {\n"
						"		%s axis= {1.0, 0.0, 0.0};\n"
						"		axis= cross_%s(axis, v1);\n"
						"		if (length_sqr_%s(axis) == 0.0) {\n"
						"			axis= (%s) {0.0, 1.0, 0.0};\n"
						"			axis= cross_%s(axis, v1);\n"
						"		}\n"
						"		return normalized_%s((%s) {axis.x, axis.y, axis.z, 0});\n"
						"	}\n"
						"	F64 mul= sqrt(2 + dot*2);\n"
						"	%s v= scaled_%s(1.0/mul, cross_%s(v1, v2));\n"
						"	return (%s) {v.x, v.y, v.z, 0.5*mul};\n}\n\n",
						q.axis_lc_name, q.axis_lc_name, q.axis_lc_name, q.lc_name, q.axis_type_name,
						q.axis_lc_name, q.axis_lc_name, q.axis_type_name, q.axis_lc_name, q.lc_name,
						q.name, q.axis_type_name, q.axis_lc_name, q.axis_lc_name, q.name);
						

			fprintf(f, "static\n");
			fprintf(f, "%s %s_by_xy_rot_matrix(%s cs, %s sn)\n", q.name, q.lc_name, q.comp_type_name, q.comp_type_name);
			fprintf(f, "{\n"
						"	/// @todo There must be a faster way\n"
						"	F64 rot= atan2(sn, cs);\n"
						"	return (%s) {0, 0, sin(rot/2.0), cos(rot/2.0) };\n"
						"}\n\n", q.name);

			fprintf(f, "static\n");
			fprintf(f, "%s lerp_%s(%s a, %s b, %s t)\n", q.name, q.lc_name, q.name, q.name, q.comp_type_name);
			fprintf(f, "{ return (%s) {", q.name);
			for (int k= 0; k < 4; ++k)
				fprintf(f, "a.%s*(1 - t) + b.%s*t, ", comp_names[k], comp_names[k]);
			fprintf(f, "}; }\n\n");
		}
		fprintf(f,
			"static\n"
			"Qf qd_to_qf(Qd q)\n"
			"{ return (Qf) {q.x, q.y, q.z, q.w}; }\n");

		fprintf(f,
			"static\n"
			"Qd qf_to_qd(Qf q)\n"
			"{ return (Qd) {q.x, q.y, q.z, q.w}; }\n");

		fprintf(f, "#endif\n");

		fclose(f);
	}
}

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4) {
		printf("Wrong number of args\n");
		return 1;
	}

	const char *src_file= argv[1];
	const char *dst_file= argv[2];
	int rtti_only= argc == 4 && !strcmp(argv[3], "rtti_only");

	if (!rtti_only)
		write_math();

	write_rtti(src_file, dst_file);
	return 0;
}

