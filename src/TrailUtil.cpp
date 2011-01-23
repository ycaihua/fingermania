#include "global.h"
#include "TrailUtil.h"
#include "Trail.h"
#include "Course.h"
#include "XmlFile.h"
#include "GameManager.h"
#include "Song.h"


int TrailUtil::GetNumSongs( const Trail *pTrail )
{
	return pTrail->m_vEntries.size();
}

float TrailUtil::GetTotalSeconds( const Trail *pTrail )
{
	float fSecs = 0;
	FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
		fSecs += e->pSong->m_fMusicLengthSeconds;
	return fSecs;
}

void TrailID::FromTrail( const Trail *p )
{
	if( p == NULL )
	{
		st = StepsType_Invalid;
		cd = Difficulty_Invalid;
	}
	else
	{
		st = p->m_StepsType;
		cd = p->m_CourseDifficulty;
	}
	m_Cache.Unset();
}

Trail *TrailID::ToTrail( const Course *p, bool bAllowNull ) const
{
	ASSERT( p );

	Trail *pRet = NULL;
	if( !m_Cache.Get(&pRet) )
	{
		if( st != StepsType_Invalid && cd != Difficulty_Invalid )
			pRet = p->GetTrail( st, cd );
		m_Cache.Set( pRet );
	}

	if( !bAllowNull && pRet == NULL )
		RageException::Throw( "%i, %i, \"%s\"", st, cd, p->GetDisplayFullTitle().c_str() );	

	return pRet;
}

XNode* TrailID::CreateNode() const
{
	XNode* pNode = new XNode( "Trail" );

	pNode->AppendAttr( "StepsType", GAMEMAN->GetStepsTypeInfo(st).szName );
	pNode->AppendAttr( "CourseDifficulty", DifficultyToString(cd) );

	return pNode;
}

void TrailID::LoadFromNode( const XNode* pNode ) 
{
	ASSERT( pNode->GetName() == "Trail" );

	RString sTemp;

	pNode->GetAttrValue( "StepsType", sTemp );
	st = GAMEMAN->StringToStepsType( sTemp );

	pNode->GetAttrValue( "CourseDifficulty", sTemp );
	cd = StringToDifficulty( sTemp );
	m_Cache.Unset();
}

RString TrailID::ToString() const
{
	RString s = GAMEMAN->GetStepsTypeInfo(st).szName;
	s += " " + DifficultyToString( cd );
	return s;
}

bool TrailID::IsValid() const
{
	return st != StepsType_Invalid && cd != Difficulty_Invalid;
}

bool TrailID::operator<( const TrailID &rhs ) const
{
#define COMP(a) if(a<rhs.a) return true; if(a>rhs.a) return false;
	COMP(st);
	COMP(cd);
#undef COMP
	return false;
}

#include "LuaBinding.h"

namespace
{
	int GetNumSongs( lua_State *L )
	{
		Trail *pTrail = Luna<Trail>::check( L, 1, true );
		int iNum = TrailUtil::GetNumSongs( pTrail );
		LuaHelpers::Push( L, iNum );
		return 1;
	}
	int GetTotalSeconds( lua_State *L )
	{
		Trail *pTrail = Luna<Trail>::check( L, 1, true );
		float fSecs = TrailUtil::GetTotalSeconds( pTrail );
		LuaHelpers::Push( L, fSecs );
		return 1;
	}

	const luaL_Reg TrailUtilTable[] =
	{
		LIST_METHOD( GetNumSongs ),
		LIST_METHOD( GetTotalSeconds ),
		{ NULL, NULL }
	};
}

LUA_REGISTER_NAMESPACE( TrailUtil )
