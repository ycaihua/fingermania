#include "global.h"

#include "RageTexture.h"
#include "RageUtil.h"
#include "RageTextureManager.h"
#include <cstring>

RageTexture::RageTexture(RageTextureID name) : 
	m_ID(name)
{
	m_iRefCount = 1;
	m_bWasUsed = false;

	m_iSourceWidth = m_iSourceHeight = 0;
	m_iTextureWidth = m_iTextureHeight = 0;
	m_iImageWidth = m_iImageHeight = 0;
	m_iFramesWide = m_iFramesHigh = 1;
}

RageTexture::~RageTexture()
{
}

void RageTexture::CreateFrameRects()
{
	GetFrameDimensionsFromFileName(GetID().filename, &m_iFramesWide, &m_iFramesHigh);

	m_TextureCoordRects.clear();

	for (int j = 0; j < m_iFramesHigh; j++)
	{
		for (int i = 0; i < m_iFramesWide; i++)
		{
			RectF frect((i + 0) / (float)m_iFramesWide * m_iImageWidth / (float)m_iTextureWidth,
						(j + 0) / (float)m_iFramesHigh * m_iImageHeight / (float)m_iTextureHeight,
						(i + 1) / (float)m_iFramesWide * m_iImageWidth / (float)m_iTextureWidth,
						(j + 1) / (float)m_iFramesHigh * m_iImageHeight / (float)m_iTextureHeight);
			m_TextureCoordRects.push_back(frect);
		}
	}
}

void RageTexture::GetFrameDimensionsFromFileName(RString sPath, int* piFramesWide, int* piFramesHigh)
{
	static Regex match(" ([0-9]+)x([0-9]+)([\\. ]|$)");
	vector<RString> asMatch;
	if (!match.Compare(sPath, asMatch))
	{
		*piFramesWide = *piFramesHigh = 1;
		return;
	}
	*piFramesWide = atoi(asMatch[0]);
	*piFramesHigh = atoi(asMatch[1]);
}

const RectF* RageTexture::GetTextureCoordRect(int iFrameNo) const
{
	return &m_TextureCoordRects[iFrameNo];
}

#include "LuaBinding.h"

class LunaRageTexture : public Luna<RageTexture>
{
public:
	static int position(T* p, lua_State* L)
	{
		p->SetPosition(FArg(1));
		return 0;
	}

	static int loop(T* p, lua_State* L)
	{
		p->SetLooping(BIArg(1));
		return 0;
	}

	static int rate(T* p, lua_State* L)
	{
		p->SetPlaybackRate(FArg(1));
		return 0;
	}

	static int GetTextureCoordRect(T* p, lua_State* L)
	{
		const RectF* pRect = p->GetTextureCoordRect(IArg(1));
		lua_pushnumber(L, pRect->left);
		lua_pushnumber(L, pRect->top);
		lua_pushnumber(L, pRect->right);
		lua_pushnumber(L, pRect->bottom);
		return 4;
	}

	LunaRageTexture()
	{
		ADD_METHOD(position);
		ADD_METHOD(loop);
		ADD_METHOD(rate);
		ADD_METHOD(GetTextureCoordRect);
	}
};

LUA_REGISTER_CLASS(RageTexture)
