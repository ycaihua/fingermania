#include "global.h"
#include "Transition.h"
#include "ScreenManager.h"

Transition::Transition()
{
	m_State = waiting;
}

void Transition::Load( RString sBGAniDir )
{
	this->RemoveAllChildren();

	m_sprTransition.Load( sBGAniDir );
	this->AddChild( m_sprTransition );

	m_State = waiting;
}


void Transition::UpdateInternal( float fDeltaTime )
{
	if( m_State != transitioning )
		return;

	if( m_sprTransition->GetTweenTimeLeft() == 0 )
	{
		m_State = finished;
		SCREENMAN->SendMessageToTopScreen( m_MessageToSendWhenDone );
	}

	ActorFrame::UpdateInternal( fDeltaTime );
}

void Transition::Reset()
{
	m_State = waiting;
	m_bFirstUpdate = true;

	if( m_sprTransition.IsLoaded() )
		m_sprTransition->FinishTweening();
}

bool Transition::EarlyAbortDraw() const
{
	return m_State == waiting;
}

void Transition::StartTransitioning( ScreenMessage send_when_done )
{
	if( m_State != waiting )
		return;	
	
	if( !m_sprTransition.IsLoaded() )
		return;
	
	m_sprTransition->PlayCommand( "StartTransitioning" );

	m_MessageToSendWhenDone = send_when_done;
	m_State = transitioning;
}

float Transition::GetTweenTimeLeft() const
{
	if( m_State != transitioning )
		return 0;

	return m_sprTransition->GetTweenTimeLeft();
}

