#include "global.h"
#include "Banner.h"
#include "BannerCache.h"
#include "SongManager.h"
#include "RageUtil.h"
#include "Song.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "Character.h"
#include "ThemeMetric.h"
#include "CharacterManager.h"
#include "ActorUtil.h"
#include "UnlockManager.h"
#include "PrefsManager.h"

REGISTER_ACTOR_CLASS(Banner)

ThemeMetric<bool> SCROLL_RANDOM("Banner", "ScrollRandom");
ThemeMetric<bool> SCROLL_ROULETTE("Banner", "ScrollRoulette");

Banner::Banner()
{
	m_bScrolling = false;
	m_fPercentScrolling = 0;
}

void Banner::Load(RageTextureID ID, bool bIsBanner)
{
	if (ID.filename == "")
	{
		LoadFallback();
		return;
	}

	if (bIsBanner)
		ID = SongBannerTexture(ID);

	m_fPercentScrolling = 0;
	m_bScrolling = false;

	TEXTUREMAN->DisableOldDimensionWarning();
	TEXTUREMAN->VolatileTexture(ID);
	Sprite::Load(ID);
	TEXTUREMAN->EnableOddDimensionWarning();
};

void Banner::LoadFromCachedBanner(const RString& sPath)
{
	if (sPath.empty())
	{
		LoadFallback();
		return;
	}

	RageTextureID ID;
	bool bLowRes = (PREFSMAN->m_BannerCache != BNCACHE_FULL);
	if (!bLowRes)
	{
		ID = Sprite::SongBannerTexture(sPath);
	}
	else
	{
		ID = BANNERCACHE->LoadCachedBanner(sPath);
	}

	if (TEXTUREMAN->IsTextureRegistered(ID))
		Load(ID);
	else if (IsAFile(sPath))
		Load(sPath);
	else
		LoadFallback();
}

void Banner::Update(float fDeltaTime)
{
	Sprite::Update(fDeltaTime);

	if (m_bScrolling)
	{
		m_fPercentScrolling += fDeltaTime / 2;
		m_fPercentScrolling -= (int)m_fPercentScrolling;

		const RectF* pTextureRect = GetCurrentTextureCoordRect();

		float fTexCoords[8] = 
		{
			0 + m_fPercentScrolling, pTextureRect->top,
			0 + m_fPercentScrolling, pTextureRect->bottom,
			1 + m_fPercentScrolling, pTextureRect->bottom,
			1 + m_fPercentScrolling, pTextureRect->top,
		};
		Sprite::SetCustomTextureCoords(fTexCoords);
	}
}

void Banner::SetScrolling(bool bScroll, float Percent)
{
	m_bScrolling = bScroll;
	m_fPercentScrolling = Percent;

	Update(0);
}

void Banner::LoadFromSong(Song* pSong)
{
	if (pSong == NULL) LoadFallback();
	else if (pSong->HasBanner()) Load(pSong->GetBannerPath());
	else	LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadAllMusic()
{
	Load(THEME->GetPathG("Banner", "All"));
	m_bScrolling = false;
}

void Banner::LoadMode()
{
	Load(THEME->GetPathG("Banner", "Mode"));
	m_bScrolling = false;
}

void Banner::LoadFromSongGroup(RString sSongGroup)
{
	RString sGroupBannerPath = SONGMAN->GetSongGroupBannerPath(sSongGroup);
	if (sGroupBannerPath != "")
		Load(sGroupBannerPath);
	else
		LoadFallback();
	m_bScrolling = false;
}

void Banner::LoadFromCourse(const Course* pCourse)
{
	if (pCourse == NULL)
		LoadFallback();
	else if (pCourse->GetBannerPath() != "")
		Load(pCourse->GetBannerPath());
	else
		LoadCourseFallback();

	m_bScrolling = false;
}

void Banner::LoadCardFromCharacter(const Character* pCharacter)
{
	if (pCharacter == NULL)
		LoadFallback();
	else if (pCharacter->GetCardPath() != "")
		Load(pCharacter->GetCardPath());
	else
		LoadFallback();
	m_bScrolling = false;
}

void Banner::LoadIconFromCharacter(const Character* pCharacter)
{
	if (pCharacter == NULL)
		LoadFallbackCharacterIcon();
	else if (pCharacter->GetIconPath() != "")
		Load(pCharacter->GetIconPath(), false);
	else
		LoadFallbackCharacterIcon();

	m_bScrolling = false;
}

void Banner::LoadBannerFromUnlockEntry(const UnlockEntry* pUE)
{
	if (pUE == NULL)
		LoadFallback();
	else
	{
		RString sFile = pUE->GetBannerFile();
		Load(sFile);
		m_bScrolling = false;
	}
}

void Banner::LoadBackgroundFromUnlockEntry(const UnlockEntry* pUE)
{
	if (pUE == NULL)
		LoadFallback();
	else
	{
		RString sFile = pUE->GetBackgroundFile();
		Load(sFile);
		m_bScrolling = false;
	}
}

void Banner::LoadFallback()
{
	Load(THEME->GetPathG("Common", "fallback banner"));
}

void Banner::LoadCourseFallback()
{
	Load(THEME->GetPathG("Banner", "course fallback"));
}

void Banner::LoadFallbackCharacterIcon()
{
	Character* pCharacter = CHARMAN->GetDefaultCharacter();
	if (pCharacter && !pCharacter->GetIconPath().empty())
		Load(pCharacter->GetIconPath(), false);
	else
		LoadFallback();
}

void Banner::LoadRoulette()
{
	Load(THEME->GetPathG("Banner", "roulette"));
	m_bScrolling = (bool)SCROLL_RANDOM;
}

void Banner::LoadRandom()
{
	Load(THEME->GetPathG("Banner", "random"));
	m_bScrolling = (bool)SCROLL_ROULETTE;
}

#include "LuaBinding.h"

class LunaBanner : public Luna<Banner>
{
public:
	static int scaletoclipped(T* p, lua_State* L)
	{
		p->ScaleToClipped(FArg(1), FArg(2));
		return 0;
	}

	static int ScaleToClipped(T* p, lua_State* L)
	{
		p->ScaleToClipped(FArg(1), FArg(2));
		return 0;
	}

	static int LoadFromSong(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadFromSong(NULL);
		}
		else 
		{
			Song *pS = Luna<Song>::check(L, 1);
			p->LoadFromSong(pS);
		}
		return 0;
	}

	static int LoadFromCourse(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadFromCourse(NULL);
		}
		else
		{
			Course *pC = Luna<Course>::check(L, 1);
			p->LoadFromCourse(pC);
		}

		return 0;
	}

	static int LoadFromCachedBanner(T* p, lua_State* L)
	{
		p->LoadFromCachedBanner(SArg(1));
		return 0;
	}

	static int LoadIconFromCharacter(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadIconFromCharacter(NULL);
		}
		else
		{
			Character* pC = Luna<Character>::check(L, 1);
			p->LoadIconFromCharacter(pC);
		}
		return 0;
	}

	static int LoadCardFromCharacter(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadIconFromCharacter(NULL);
		}
		else
		{
			Character* pC = Luna<Character>::check(L, 1);
			p->LoadIconFromCharacter(pC);
		}
		return 0;
	}

	static int LoadBannerFromUnlockEntry(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadBannerFromUnlockEntry(NULL);
		}
		else
		{
			UnlockEntry* pUE = Luna<UnlockEntry>::check(L, 1);
			p->LoadBannerFromUnlockEntry(pUE);
		}
		return 0;
	}

	static int LoadBackgroundFromUnlockEntry(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->LoadBackgroundFromUnlockEntry(NULL);
		}
		else
		{
			UnlockEntry* pUE = Luna<UnlockEntry>::check(L, 1);
			p->LoadBackgroundFromUnlockEntry(pUE);
		}
		return 0;
	}

	static int LoadFromSongGroup(T* p, lua_State* L)
	{
		p->LoadFromSongGroup(SArg(1));
		return 0;
	}

	LunaBanner()
	{
		ADD_METHOD(scaletoclipped);
		ADD_METHOD(ScaleToClipped);
		ADD_METHOD(LoadFromSong);
		ADD_METHOD(LoadFromCourse);
		ADD_METHOD(LoadFromCachedBanner);
		ADD_METHOD(LoadIconFromCharacter);
		ADD_METHOD(LoadCardFromCharacter);
		ADD_METHOD(LoadBannerFromUnlockEntry);
		ADD_METHOD(LoadBackgroundFromUnlockEntry);
		ADD_METHOD(LoadFromSongGroup);
	}
};

LUA_REGISTER_DERIVED_CLASS(Banner, Sprite);
