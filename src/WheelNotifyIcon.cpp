#include "global.h"
#include "WheelNotifyIcon.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "WheelNotifyIcon.h"
#include "RageTimer.h"
#include "ThemeManager.h"

static ThemeMetric<bool> SHOW_TRAINING("Wheel_NotifyIcon", "ShowTraining");

WheelNotifyIcon::WheelNotifyIcon()
{
	Load(THEME->GetPathG("WheelNotifyIcon", "icon 4x2"));
	StopAnimating();
}

void WheelNotifyIcon::SetFlags(Flags flags)
{
	m_vIconsToShow.clear();

	switch(flags.iPlayersBestNumber)
	{
	case 1:
		m_vIconsToShow.push_back(best1);
		break;
	case 2:
		m_vIconsToShow.push_back(best2);
		break;
	case 3:
		m_vIconsToShow.push_back(best3);
		break;
	}

	if (flags.bEdits)
		m_vIconsToShow.push_back(edits);

	switch(flags.iStagesForSong)
	{
	case 1:
		break;
	case 2:
		m_vIconsToShow.push_back(long_ver);
		break;
	case 3:
		m_vIconsToShow.push_back(marathon);
		break;
	default:
		FAIL_M(ssprintf("flags.iStagesForSong = %d", flags.iStagesForSong));
	}

	if (flags.bHasBeginnerOr1Meter && (bool)SHOW_TRAINING)
		m_vIconsToShow.push_back(training);

	if (m_vIconsToShow.size() == 1)
	{
		if (m_vIconsToShow[0] >= best1 && m_vIconsToShow[0] <= best3)
			m_vIconsToShow.push_back(empty);
	}

	m_vIconsToShow.resize(min(m_vIconsToShow.size(), 2u));

	Update(0);
}

bool WheelNotifyIcon::EarlyAbortDraw() const
{
	if (m_vIconsToShow.empty())
		return true;
	return Sprite::EarlyAbortDraw();
}

void WheelNotifyIcon::Update(float fDeltaTime)
{
	if (m_vIconsToShow.size() > 0)
	{
		const float fSecondFraction = fmodf(RageTimer::GetTimeSinceStartFast(), 1);
		const int index = (int)(fSecondFraction * m_vIconsToShow.size());
		Sprite::SetState(m_vIconsToShow[index]);
	}

	Sprite::Update(fDeltaTime);
}
