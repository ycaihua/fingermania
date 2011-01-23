#include "global.h"

#include "RageDisplay.h"
#include "RageDisplay_Null.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "RageUtil.h"
#include "RageSurface.h"
#include "DisplayResolutions.h"

static RageDisplay::PixelFormatDesc PIXEL_FORMAT_DESC[NUM_PixelFormat] = 
{
	{
		32,
		{ 0xFF000000,
		  0x00FF0000,
		  0x0000FF00,
		  0x000000FF }
	}, {
		16,
		{ 0xF000,
		  0x0F00,
		  0x00F0,
		  0x000F },
	}, {
		16,
		{ 0xF800,
		  0x07C0,
		  0x003E,
		  0x0001 },
	}, {
		16,
		{ 0xF800,
		  0x07C0,
		  0x003E,
		  0x0000 },
	}, {
		24,
		{ 0xFF0000,
		  0x00FF00,
		  0x0000FF,
		  0x000000 }
	}, {
		8,
		{ 0,0,0,0 }
	}, {

		24,
		{ 0x0000FF,
		  0x00FF00,
		  0xFF0000,
		  0x000000 }
	}, {
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x8000 },
	}
};

RageDisplay_Null::RageDisplay_Null()
{
	LOG->MapLog("renderer", "Current renderer: null");
}

RString RageDisplay_Null::Init( const VideoModeParams &p, bool bAllowUnacceleratedRenderer )
{
	bool bIgnore = false;
	SetVideoMode( p, bIgnore );
	return RString();
}

void RageDisplay_Null::GetDisplayResolutions( DisplayResolutions &out ) const
{
	out.clear();
	DisplayResolution res = { 640, 480, true };
	out.insert( res );
}

RageSurface* RageDisplay_Null::CreateScreenshot()
{
	const PixelFormatDesc &desc = PIXEL_FORMAT_DESC[PixelFormat_RGB8];
	RageSurface *image = CreateSurface(
		640, 480, desc.bpp,
		desc.masks[0], desc.masks[1], desc.masks[2], desc.masks[3] );

	memset( image->pixels, 0, 480*image->pitch );

	return image;
}

const RageDisplay::PixelFormatDesc *RageDisplay_Null::GetPixelFormatDesc(PixelFormat pf) const
{
	ASSERT( pf >= 0 && pf < NUM_PixelFormat );
	return &PIXEL_FORMAT_DESC[pf];
}


RageMatrix RageDisplay_Null::GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf )
{
	RageMatrix m(
		2/(r-l),      0,            0,           0,
		0,            2/(t-b),      0,           0,
		0,            0,            -2/(zf-zn),   0,
		-(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn),  1 );
	return m;
}


void RageDisplay_Null::EndFrame()
{
	ProcessStatsOnFlip();
}
	

class RageCompiledGeometryNull : public RageCompiledGeometry
{
public:
	
	void Allocate( const vector<msMesh> &vMeshes ) {}
	void Change( const vector<msMesh> &vMeshes ) {}
	void Draw( int iMeshIndex ) const {}
};

RageCompiledGeometry* RageDisplay_Null::CreateCompiledGeometry()
{
	return new RageCompiledGeometryNull;
}

void RageDisplay_Null::DeleteCompiledGeometry( RageCompiledGeometry* p )
{
}
