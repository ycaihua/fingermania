#include "global.h"
#include "DifficultyIcon.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "Steps.h"
#include "GameState.h"
#include "RageDisplay.h"
#include "arch/Dialog/Dialog.h"
#include "Trail.h"
#include "ActorUtil.h"
#include "XmlFile.h"
#include "LuaManager.h"

REGISTER_ACTOR_CLASS(DifficultyIcon)

DifficultyIcon::DifficultyIcon()
{
	m_bBlank = false;
	m_PlayerNumber = PLAYER_1;
}

bool DifficultyIcon::Load(RString sPath)
{
	Sprite::Load(sPath);
	int iState = GetNumStates();
	bool bWarn = iState != NUM_Difficulty && iState != NUM_Difficulty * 2;
	if (sPath.find("_blank") != string::npos)
		bWarn = false;
	if (bWarn)
	{
		RString sError = ssprintf(
			"The difficulty icon graphic '%s' must have %d or %d frames. It has %d states.",
			sPath.c_str(),
			NUM_Difficulty,
			NUM_Difficulty * 2,
			iStates);
		Dialog::OK(sError);
	}
	StopAnimating();
	return true;
}

void DifficultyIcon::LoadFromNode(const XNode* pNode)
{
	RString sFile;
	if (!ActorUtil::GetAttrPath(pNode, "File", sFile))
		RageException::Throw("%s: DifficultyIcon: missing the \"File\" attribute.", ActorUtil::GetWhere(pNode).c_str());

	Load(sFile);

	Actor::LoadFromNode(pNode);
}

void DifficultyIcon::SetPlayer(PlayerNumber pn)
{
	m_PlayerNumber = pn;
}

void DifficultyIcon::SetFromSteps(PlayerNumber pn, const Steps* pSteps)
{
	SetPlayer(pn);
	if (pSteps == NULL)
		Unset();
	else
		SetFromDifficulty(pSteps->GetDifficulty());
}

void DifficultyIcon::SetFromTrail(PlayerNumber pn, const Trail* pTrail)
{
	SetPlayer(pn);
	if (pTrail == NULL)
		Unset();
	else
		SetFromDifficulty(pTrail->m_CourseDifficulty);
}

void DifficultyIcon::Unset()
{
	m_bBlank = true;
}

void DifficultyIcon::SetFromDifficulty(Difficulty dc)
{
	m_bBlank = false;
	switch(GetNumStates())
	{
	case NUM_Difficulty:
		SetState(dc); 
		break;
	case NUM_Difficulty * 2:
		SetState(dc * 2 + m_PlayerNumber);
		break;
	default:
		m_bBlank = true;
		break;
	}
}

#include "LuaBinding.h"

class LunaDifficultyIcon : public Luna<DifficultyIcon>
{
public:
	static int SetFromSteps(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->Unset();
		}
		else
		{
			Steps* pS = Luna<Steps>::check(L, 1);
			p->SetFromSteps(PLAYER_1, pS);
		}
		return 0;
	}

	static int SetFromTrail(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1))
		{
			p->Unset();
		}
		else
		{
			Trail* pT = Luna<Trail>::check(L, 1);
			p->SetFromTrail(PLAYER_1, pT);
		}
		return 0;
	}
	static int Unset(T* p, lua_State* L)
	{
		p->Unset();
		return 0;
	}
	static int SetPlayer(T* p, lua_State* L)
	{
		p->SetPlayer(Enum::Check<PlayerNumber>(L, 1));
		return 0;
	}
	static int SetFromDifficulty(T* p, lua_State* L)
	{
		p->SetFromDifficulty(Enum::Check<Difficulty>(L, 1));
		return 0;
	}

	LunaDifficultyIcon()
	{
		ADD_METHOD(Unset);
		ADD_METHOD(SetPlayer);
		ADD_METHOD(SetFromSteps);
		ADD_METHOD(SetFromTrail);
		ADD_METHOD(SetFromDifficulty);
	}
};

LUA_REGISTER_DERIVED_CLASS(DifficultyIcon, Sprite)
