#include "global.h"
#include "ScoreDisplayCalories.h"
#include "MessageManager.h"
#include "PlayerNumber.h"
#include "PlayerState.h"
#include "RageUtil.h"
#include "StageStats.h"
#include "XmlFile.h"
#include "ActorUtil.h"
#include "StatsManager.h"
#include "LuaManager.h"

REGISTER_ACTOR_CLASS(ScoreDisplayCalories)

ScoreDisplayCalories::ScoreDisplayCalories()
{
}

ScoreDisplayCalories::~ScoreDisplayCalories()
{
	if (m_sMessageOnStep != "")
		MESSAGEMAN->Unsubscribe(this, m_sMessageOnStep);
}

void ScoreDisplayCalories::LoadFromNode(const XNode* pNode)
{
	RollingNumbers::LoadFromNode(pNode);

	{
		Lua* L = LUA->Get();
		bool b = pNode->PushAttrValue(L, "PlayerNumber");
		ASSERT(b);
		LuaHelpers::Pop(L, m_PlayerNumber);
		LUA->Release(L);
	}

	MESSAGEMAN->Subscribe(this, "Step");
}

void ScoreDisplayCalories::Update(float fDelta)
{
	if (IsFirstUpdate())
		UpdateNumber();

	RollingNumbers::Update(fDelta);
}

void ScoreDisplayCalories::HandleMessage(const Message& msg)
{
	if (msg.GetName() == "Step")
	{
		PlayerNumber pn;
		msg.GetParam("PlayerNumber", pn);
		if (pn == m_PlayerNumber)
			UpdateNumber();
	}

	RollingNumbers::HandleMessage(msg);
}

void ScoreDisplayCalories::UpdateNumber()
{
	float fCals = 
		STATSMAN->GetAccumPlayedStageStats().m_player[m_PlayerNumber].m_fCaloriesBurned +
		STATSMAN->m_CurStageStats.m_player[m_PlayerNumber].m_fCaloriesBurned;

	SetTargetNumber(fCals);
}

#include "LuaBinding.h"

class LunaScoreDisplayCalories : public Luna<ScoreDisplayCalories>
{
public:
	LunaScoreDisplayCalories()
	{
	}
};

LUA_REGISTER_DERIVED_CLASS(ScoreDisplayCalories, BitmapText);
