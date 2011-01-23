#include "global.h"
#include "DualScrollBar.h"
#include "ThemeManager.h"
#include "RageUtil.h"

DualScrollBar::DualScrollBar()
{
	m_fBarHeight = 100;
	m_fBarTime = 1;
}

void DualScrollBar::Load(const RString& sType)
{
	FOREACH_PlayerNumber(pn)
	{
		m_sprScrollThumbUnderHalf[pn].Load(THEME->GetPathG(sType, ssprintf("thumb p%i", pn + 1)));
		m_sprScrollThumbUnderHalf[pn]->SetName(ssprintf("ThumbP%i", pn + 1));
		this->AddChild(m_sprScrollThumbUnderHalf[pn]);
	}

	FOREACH_PlayerNumber(pn)
	{
		m_sprScrollThumbOverHalf[pn].Load(THEME->GetPathG(sType, ssprintf("thumb p%i", pn + 1)));
		m_sprScrollThumbOverHalf[pn]->SetName(ssprintf("ThumbP%i", pn + 1));
		this->AddChild(m_sprScrollThumbOverHalf[pn]);
	}

	m_sprScrollThumbUnderHalf[0]->SetCropLeft(.5f);
	m_sprScrollThumbUnderHalf[1]->SetCropRight(.5f);

	m_sprScrollThumbOverHalf[0]->SetCropRight(.5f);
	m_sprScrollThumbOverHalf[1]->SetCropLeft(.5f);

	FOREACH_PlayerNumber(pn)
		SetPercentage(pn, 0);

	FinishTweening();
}

void DualScrollBar::EnablePlayer(PlayerNumber pn, bool on)
{
	m_sprScrollThumbUnderHalf[pn]->SetVisible(on);
	m_sprScrollThumbOverHalf[pn]->SetVisible(on);
}

void DualScrollBar::SetPercentage(PlayerNumber pn, float fPercent)
{
	const float bottom = m_fBarHeight / 2 - m_sprScrollThumbUnderHalf[pn]->GetZoomedHeight() / 2;
	const float top = -bottom;

	m_sprScrollThumbUnderHalf[pn]->StopTweening();
	m_sprScrollThumbUnderHalf[pn]->BeginTweening(m_fBarTime);
	m_sprScrollThumbUnderHalf[pn]->SetY(SCALE(fPercent, 0, 1, top, bottom));

	m_sprScrollThumbOverHalf[pn]->StopTweening();
	m_sprScrollThumbOverHalf[pn]->BeginTweening(m_fBarTime);
	m_sprScrollThumbOverHalf[pn]->SetY(SCALE(fPercent, 0, 1, top, bottom));
}

