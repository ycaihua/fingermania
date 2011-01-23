#include "global.h"
#include "ScreenSplash.h"
#include "ThemeManager.h"
#include "RageUtil.h"
#include "ScreenManager.h"

REGISTER_SCREEN_CLASS( ScreenSplash );

void ScreenSplash::Init()
{
	ALLOW_START_TO_SKIP.Load( m_sName, "AllowStartToSkip" );
	PREPARE_SCREEN.Load( m_sName, "PrepareScreen" );

	ScreenWithMenuElements::Init();
}

void ScreenSplash::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();
}

void ScreenSplash::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_DoneFadingIn )
	{
		if( PREPARE_SCREEN )
			SCREENMAN->PrepareScreen( GetNextScreenName() );
	}
	else if( SM == SM_MenuTimer )
	{
		StartTransitioningScreen( SM_GoToNextScreen );
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenSplash::MenuBack( const InputEventPlus &input )
{
	Cancel( SM_GoToPrevScreen );
}

void ScreenSplash::MenuStart( const InputEventPlus &input )
{
	if( IsTransitioning() )
		return;
	if( !ALLOW_START_TO_SKIP )
		return;
	StartTransitioningScreen( SM_GoToNextScreen );
}

