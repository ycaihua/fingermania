#include "global.h"
#include "ScoreKeeper.h"
#include "NoteData.h"
#include "PlayerState.h"
#include "NoteDataWithScoring.h"

ScoreKeeper::ScoreKeeper(PlayerState* pPlayerState, PlayerStageStats* pPlayerStageStats)
{
	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
}

void ScoreKeeper::GetScoreOfLastTapInRow(const NoteData& nd, int iRow,
	TapNoteScore& tnsOut, int& iNumTapsInRowOut)
{
	int iNum = 0;

	for (int track = 0; track < nd.GetNumTracks(); ++track)
	{
		const TapNote* tn = nd.GetTapNote(track, iRow);

		if (tn.type != TapNote::tap && tn.type != TapNote::hold_head)
			continue;
		++iNum;
	}

	tnsOut = NoteDataWithScoring::LastTapNoteWithResult(nd, iRow).result.tns;
	iNumTapsInRowOut = iNum;
}

#include "ScoreKeeperNormal.h"
#include "ScoreKeeperGuitar.h"
#include "ScoreKeeperShared.h"

ScoreKeeper* ScoreKeeper::MakeScoreKeeper(RString sClassName, PlayerState* pPlayerState, PlayerStageStats* pPlayerStageStats)
{
	if (sClassName == "ScoreKeeperNormal")
		return new ScoreKeeperNormal(pPlayerState, pPlayerStageStats);
	else if (sClassName == "ScoreKeeperGuitar")
		return new ScoreKeeperGuitar(pPlayerState, pPlayerStageStats);
	else if (sClassName == "ScoreKeeperShared")
		return new ScoreKeeperShared(pPlayerState, pPlayerStageStats);
	FAIL_M(sClassName);
}
