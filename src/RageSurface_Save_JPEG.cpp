#include "global.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_JPEG.h"

#include "RageUtil.h"
#include "RageFile.h"

#undef FAR
namespace jpeg
{
	extern "C"
	{
#include <jpeglib.h>
	}
}

#ifdef _XBOX
#pragma comment(lib, "libjpeg/xboxjpeg.lib")
#elif defined _MSC_VER
#pragma comment(lib, "libjpeg/jpeg.lib")
#endif

#define OUTPUT_BUFFER_SIZE 4096
typedef struct
{
	struct jpeg::jpeg_destination_mgr pub;

	RageFile* f;
	uint8_t buffer[OUTPUT_BUFFER_SIZE];
} my_destination_mgr;

static void init_destination(jpeg::j_compress_ptr cinfo)
{
	return;
}

static jpeg::boolean empty_output_buffer(jpeg::j_compress_ptr cinfo)
{
	my_destination_mgr* dest = (my_destination_mgr*)cinfo->dest;
	dest->f->Write(dest->buffer, OUTPUT_BUFFER_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;

	return TRUE;
}

static void term_destination(jpeg::j_compress_ptr cinfo)
{
	my_destination_mgr* dest = (my_destination_mgr*)cinfo->dest;
	dest->f->Write( dest->buffer, OUTPUT_BUFFER_SIZE - dest->pub.free_in_buffer );
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;
}

static void jpeg_RageFile_dest(jpeg::j_compress_ptr cinfo, RageFile& f)
{
	ASSERT(cinfo->dest == NULL);

	cinfo->dest = (struct jpeg::jpeg_destination_mgr*)
		(*cinfo->mem->alloc_small)((jpeg::j_common_ptr)cinfo, JPOOL_PERMANENT,
		sizeof(my_destination_mgr));

	my_destination_mgr* dest = (my_destination_mgr*)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;
	dest->pub.next_output_byte = dest->buffer;

	dest->f = &f;
}

bool RageSurfaceUtils::SaveJPEG(RageSurface* surface, RageFile& f, bool bHighQual)
{
	RageSurface* dst_surface;
	if (RageSurfaceUtils::ConvertSurface(surface, dst_surface,
		surface->w, surface->h, 24, Swap24BE(0xFF0000), Swap24BE(0x00FF00), Swap24BE(0x0000FF), 0))
		surface = dst_surface;

	struct jpeg::jpeg_compress_struct cinfo;
	struct jpeg::jpeg_error_mgr jerr;
	cinfo.err = jpeg::jpeg_std_error(&jerr);

	jpeg::jpeg_CreateCompress(&cinfo, JPEG_LIB_VERSION, \
		(size_t)sizeof(struct jpeg::jpeg_compress_struct));

	cinfo.image_width = surface->w;
	cinfo.image_height = surface->h;
	cinfo.input_components = 3;
	cinfo.in_color_space = jpeg::JCS_RGB;

	jpeg::jpeg_set_defaults(&cinfo);

	if (bHighQual)
		jpeg::jpeg_set_quality(&cinfo, 150, TRUE);
	else
		jpeg::jpeg_set_quality(&cinfo, 70, TRUE);

	jpeg_RageFile_dest(&cinfo, f);

	jpeg::jpeg_start_compress(&cinfo, TRUE);

	const int row_stride = surface->pitch;
	while (cinfo.next_scanline < cinfo.image_height)
	{
		jpeg::JSAMPROW row_pointer = &((jpeg::JSAMPLE*)surface->pixels)[cinfo.next_scaleline * row_stride];
		jpeg::jpeg_write_scanline(&cinfo, &row_pointer, 1);
	}

	jpeg::jpeg_finish_compress(&cinfo);
	jpeg::jpeg_destroy_compress(&cinfo);

	delete dst_surface;
	return true;
}
