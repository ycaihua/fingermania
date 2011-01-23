#ifndef RAGE_SURFACE_UTILS_PALETTIZE
#define RAGE_SURFACE_UTILS_PALETTIZE

struct RageSurface;
namespace RageSurfaceUtils
{
	void Palettize( RageSurface *&pImg, int iColors=256, bool bDither=true );
};

#endif
