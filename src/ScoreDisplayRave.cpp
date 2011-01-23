#include "global.h"
#include "ScoreDisplayRave.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "ActorUtil.h"
#include "PlayerState.h"

ScoreDisplayRave::ScoreDisplayRave()
{
	LOG->Trace("ScoreDisplayRave::ScoreDisplayRave()");
	m_lastLevelSeen = ATTACK_LEVEL_1;

	for (int i = 0; i < NUM_ATTACK_LEVELS; i++)
	{
		m_sprMeter[i].Load(THEME->GetPathG("ScoreDisplayRave", ssprintf("stream level%d", i + 1)));
		m_sprMeter[i].SetCropRight(1.f);
	}

	m_textLevel.LoadFromFont(THEME->GetPathF("ScoreDisplayRave", "level"));
	m_textLevel.SetText("1");
}

void ScoreDisplayRave::Init(const PlayerState* pPlayerState, const PlayerStageStats* pPlayerStageStats)
{
	ScoreDisplay::Init(pPlayerState, pPlayerStageStats);

	PlayerNumber pn = pPlayerState->m_PlayerNumber;

	m_sprFrameBase.Load(THEME->GetPathG("ScoreDisplayRave", ssprintf("frame base p%d", pn + 1)));
	this->AddChild(m_sprFrameBase);

	for (int i = 0; i < NUM_ATTACK_LEVELS; i++)
	{
		m_sprMeter[i].SetName(ssprintf("MeterP%d", pn + 1));
		LOAD_ALL_COMMANDS(m_sprMeter[i]);
		ON_COMMAND(m_sprMeter[i]);
		this->AddChild(&m_sprMeter[i]);
	}

	m_textLevel.SetName(ssprintf("LevelP%d", pn + 1));
	LOAD_ALL_COMMANDS(m_textLevel);
	ON_COMMAND(m_textLevel);
	this->AddChild(&m_textLevel);

	m_sprFrameOverlay.Load(THEME->GetPathG("ScoreDisplayRave", ssprintf("frame overlay p%d", pn + 1)));
	this->AddChild(m_sprFrameOverlay);
}

void ScoreDisplayRave::Update(float fDelta)
{
	ScoreDisplay::Update(fDelta);

	float fLevel = m_pPlayerState->m_fSuperMeter;
	AttackLevel level = (AttackLevel)(int)fLevel;

	if (level != m_lastLevelSeen)
	{
		m_sprMeter[m_lastLevelSeen].SetCropRight(1.f);
		m_lastLevelSeen = level;
		m_textLevel.SetText(ssprintf("%d", level + 1));
	}

	float fPercent = fLevel - level;
	m_sprMeter[level].SetCropRight(1 - fPercent);
}
