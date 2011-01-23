#include "global.h"
#include "ReceptorArrow.h"
#include "GameState.h"
#include "NoteSkinManager.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "Style.h"
#include "PlayerState.h"

ReceptorArrow::ReceptorArrow()
{
	m_bIsPressed = false;
	m_bWasPressed = false;
	m_bWasReverse = false;
}

void ReceptorArrow::Load( const PlayerState* pPlayerState, int iColNo )
{
	m_pPlayerState = pPlayerState;
	m_iColNo = iColNo;

	RString sButton = GAMESTATE->GetCurrentStyle()->ColToButtonName( iColNo );
	m_pReceptor.Load( NOTESKIN->LoadActor(sButton, "Receptor") );
	this->AddChild( m_pReceptor );

	bool bReverse = m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(m_iColNo) > 0.5f;
	m_pReceptor->PlayCommand( bReverse? "ReverseOn":"ReverseOff" );
	m_bWasReverse = bReverse;
}

void ReceptorArrow::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	bool bReverse = m_pPlayerState->m_PlayerOptions.GetCurrent().GetReversePercentForColumn(m_iColNo) > 0.5f;
	if( bReverse != m_bWasReverse )
	{
		m_pReceptor->PlayCommand( bReverse? "ReverseOn":"ReverseOff" );
		m_bWasReverse = bReverse;
	}
}

void ReceptorArrow::DrawPrimitives()
{
	if( m_bWasPressed  &&  !m_bIsPressed )
		m_pReceptor->PlayCommand( "Lift" );
	else if( !m_bWasPressed  &&  m_bIsPressed )
		m_pReceptor->PlayCommand( "Press" );

	m_bWasPressed = m_bIsPressed;
	m_bIsPressed = false;

	ActorFrame::DrawPrimitives();
}

void ReceptorArrow::Step( TapNoteScore score )
{
	m_bIsPressed = true;

	RString sJudge = TapNoteScoreToString( score );
	m_pReceptor->PlayCommand( Capitalize(sJudge) );
}

void ReceptorArrow::SetNoteUpcoming( bool b )
{
	m_pReceptor->PlayCommand( b ? "ShowNoteUpcoming" : "HideNoteUpcoming" );
}

