#include "global.h"
#include "Attack.h"
#include "GameState.h"
#include "RageUtil.h"
#include "Song.h"
#include "Foreach.h"
#include "PlayerOptions.h"
#include "PlayerState.h"

void Attack::GetAttackBeats(const Song* pSong, float& fStartBeat, float& fEndBeat) const
{
	ASSERT(pSong);
	ASSET_M(fStartSecond >= 0, ssprintf("%f", fStartSecond));

	fStartBeat = pSong->GetBeatFromElapsedTime(fStartSecond);
	fEndBeat = pSong->GetBeatFromElapsedTime(fStartSecond + fSecsRemaining);
}

void Attack::GetRealtimeAttackBeats(const Song* pSong, const PlayerState** pPlayerState, float& fStartBeat, float& fEndBeat) const
{
	if (fStartSecond >= 0)
	{
		GetAttackBeats(pSong, fStartBeat, fEndBeat);
		return;
	}

	ASSERT(pPlayerState);
	ASSERT(pSong);

	fStartBeat = min( GAMESTATE->m_fSongBeat+8, pPlayerState->m_fLastDrawnBeat );
	fStartBeat = truncf(fStartBeat) + 1;

	const float fStartSecond = pSong->GetElapsedTimeFromBeat(fStartBeat);
	const float fEndSecond = fStartSecond + fSecsRemaining;
	fEndBeat = pSong->GetBeatFromElapsedTime(fEndSecond);
	fEndBeat = truncf(fEndBeat) + 1;

	ASSERT_M(fEndBeat >= fStartBeat, ssprintf("%f >= %f", fEndBeat, fStartBeat));
}

bool Attack::operator==(const Attack& rhs) const
{
#define EQUAL(a) (a == rhs.a)
	return
		EQUAL(level) &&
		EQUAL(fStartSecond) &&
		EQUAL(fSecsRemaining) &&
		EQUAL(sModifiers) &&
		EQUAL(bOn) &&
		EQUAL(bGlobal);
}

bool Attack::ContainsTransformOrTurn() const
{
	PlayerOptions po;
	po.FromString(sModifiers);
	return po.ContainsTransformOrTurn();
}

Attack Attack::FromGlobalCourseModifier(const RString &sModifiers)
{
	Attack a;
	a.fStartSecond = 0;
	a.fSecsRemaining = 10000;
	a.level = ATTACK_LEVEL_1;
	a.sModifiers = sModifiers;
	a.bGlobal = true;
	return a;
}

RString Attack::GetNextDescription() const
{
	RString s = sModifiers + " " + ssprintf("(%.2f seconds)", fSecsRemaining);
	return s;
}

bool AttackArray::ContainsTransformOrTurn() const
{
	FOREACH_CONST(Attack, *this, a)
	{
		if (a->ContainsTransformOrTurn())
			return true;
	}
	return false;
}


