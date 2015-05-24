#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/string.h"
#include "platform/gl.h"
#include "resources/resblob.h"
#include "texture.h"

#ifndef CODEGEN
#	include <lodepng/lodepng.h>
#endif

internal
U32 ipow(U32 base, U32 exp)
{
	U32 result= 1;
	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}
	return result;
}

Texel * texture_texels(const Texture *tex, U32 lod)
{
	ensure(lod < tex->lod_count);
	return blob_ptr(&tex->res, tex->texel_offsets[lod]);
}

V2i lod_reso(V2i base, U32 lod)
{
	return (V2i) {
		base.x/ipow(2, lod), // @todo Should probably match GL mipmap sizes
		base.y/ipow(2, lod)
	};
}

int json_texture_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	U8 *loaded_image= NULL;
	Texel *image= NULL;
#define MAX_MIP_COUNT (MAX_TEXTURE_LOD_COUNT - 1)
	Texel *mips[MAX_MIP_COUNT]= {};
	U32 mip_byte_counts[MAX_MIP_COUNT]= {};

	JsonTok j_file= json_value_by_key(j, "file");
	if (json_is_null(j_file))
		RES_ATTRIB_MISSING("file");

	char total_path[MAX_PATH_SIZE];
	joined_path(total_path, j.json_path, json_str(j_file));

	const int comps= 4;
	U32 width, height;
	int err=
		lodepng_decode32_file(&loaded_image, &width, &height, total_path);
	if (err) {
		critical_print(	"PNG load error for '%s': %s",
						total_path, lodepng_error_text(err));
		goto error;
	}
	ensure(width > 0 && height > 0);

	// Invert y-axis
	
	image= malloc(width*height*comps);
	for (U32 y= 0; y < height; ++y) {
		memcpy(	image + y*width,
				loaded_image + (height - 1 - y)*width*comps,
				width*comps);
	}	

	const U32 lod_count= 2; // @todo Choose wisely
	ensure(lod_count <= MAX_TEXTURE_LOD_COUNT);

	Texture tex= {
		.reso= {width, height},
		.lod_count= lod_count,
	};

	// Calculate mip-map data and offsets to it
	U32 offset= buf->offset + sizeof(tex) - sizeof(tex.res);
	for (U32 lod_i= 0; lod_i < lod_count; ++lod_i) {
		tex.texel_offsets[lod_i]= offset;

		const V2i reso= lod_reso(tex.reso, lod_i);
		const U32 byte_count= reso.x*reso.y*sizeof(Texel);
		offset += byte_count;

		if (lod_i == 0) // Not a mip-level
			continue;

		// Calculate mip-maps
		const U32 mip_i= lod_i - 1;
		mips[mip_i]= malloc(byte_count);
		mip_byte_counts[mip_i]= byte_count;
		for (int y= 0; y < reso.y; ++y) {
			for (int x= 0; x < reso.x; ++x) {

				// @todo Better filter
				// Gamma == 2 for fast gamma correction
				Color avg= {};
				const V2i sample_d[4]= {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
				for (U32 sample_i= 0; sample_i < 4; ++sample_i) {
					V2i image_p= {
						x*width/reso.x + sample_d[sample_i].x,
						y*height/reso.y + sample_d[sample_i].y
					};
					image_p.x= CLAMP(image_p.x, 0, (int)width);
					image_p.y= CLAMP(image_p.y, 0, (int)height);
					const U32 texel_i= image_p.x + image_p.y*width;

					avg.r += SQR(image[texel_i].r)/4.0;
					avg.g += SQR(image[texel_i].g)/4.0;
					avg.b += SQR(image[texel_i].b)/4.0;
					avg.a += image[texel_i].a/4.0;
				}

				const U32 texel_i= x + y*reso.x;
				ensure(texel_i*sizeof(Texel) < byte_count);
				mips[mip_i][texel_i].r= sqrt(avg.r);
				mips[mip_i][texel_i].g= sqrt(avg.g);
				mips[mip_i][texel_i].b= sqrt(avg.b);
				mips[mip_i][texel_i].a= avg.a;
			}
		}
	}

	blob_write(buf, (U8*)&tex + sizeof(tex.res), sizeof(tex) - sizeof(tex.res));
	blob_write(buf, image, width*height*comps);
	for (U32 i= 0; i < MAX_MIP_COUNT && mips[i]; ++i)
		blob_write(buf, mips[i], mip_byte_counts[i]);

cleanup:
	free(loaded_image);
	free(image);
	for (U32 i= 0; i < MAX_MIP_COUNT; ++i)
		free(mips[i]);

	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

