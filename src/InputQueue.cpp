#include "global.h"
#include "InputQueue.h"
#include "RageTimer.h"
#include "RageLog.h"
#include "InputEventPlus.h"
#include "Foreach.h"
#include "InputMapper.h"


InputQueue*	INPUTQUEUE = NULL;

const unsigned MAX_INPUT_QUEUE_LENGTH = 32;

InputQueue::InputQueue()
{
	FOREACH_ENUM( GameController, gc )
		m_aQueue[gc].resize( MAX_INPUT_QUEUE_LENGTH );
}

void InputQueue::RememberInput( const InputEventPlus &iep )
{
	if( !iep.GameI.IsValid() )
		return;

	int c = iep.GameI.controller;
	if( m_aQueue[c].size() >= MAX_INPUT_QUEUE_LENGTH )	
		m_aQueue[c].erase( m_aQueue[c].begin(), m_aQueue[c].begin() + (m_aQueue[c].size()-MAX_INPUT_QUEUE_LENGTH+1) );
	m_aQueue[c].push_back( iep );
}

bool InputQueue::WasPressedRecently( GameController c, const GameButton button, const RageTimer &OldestTimeAllowed, InputEventPlus *pIEP )
{
	for( int queue_index=m_aQueue[c].size()-1; queue_index>=0; queue_index-- )	
	{
		const InputEventPlus &iep = m_aQueue[c][queue_index];
		if( iep.DeviceI.ts < OldestTimeAllowed )	
			return false;

		if( iep.GameI.button != button )
			continue;

		if( pIEP != NULL )
			*pIEP = iep;

		return true;
	}

	return false;	
}

void InputQueue::ClearQueue( GameController c )
{
	m_aQueue[c].clear();
}

static const float g_fSimultaneousThreshold = 0.05f;

bool InputQueueCode::EnteredCode( GameController controller ) const
{
	if( controller == GameController_Invalid )
		return false;
	if( m_aPresses.size() == 0 )
		return false;

	RageTimer OldestTimeAllowed;
	if( m_fMaxSecondsBack == -1 )
		OldestTimeAllowed.SetZero();
	else
		OldestTimeAllowed += -m_fMaxSecondsBack;

	
	int iSequenceIndex = m_aPresses.size()-1;	
	const vector<InputEventPlus> &aQueue = INPUTQUEUE->GetQueue( controller );
	int iQueueIndex = aQueue.size()-1;
	while( iQueueIndex >= 0 )
	{
		if( !OldestTimeAllowed.IsZero() && aQueue[iQueueIndex].DeviceI.ts < OldestTimeAllowed )
			return false;

		const ButtonPress &Press = m_aPresses[iSequenceIndex];
		if( !Press.m_InputTypes[aQueue[iQueueIndex].type] )
		{
			--iQueueIndex;
			continue;
		}

		RageTimer OldestTimeAllowedForTap( aQueue[iQueueIndex].DeviceI.ts );
		OldestTimeAllowedForTap += -g_fSimultaneousThreshold;

		bool bMatched = false;
		int iMinSearchIndexUsed = iQueueIndex;
		for( int b=0; b<(int) Press.m_aButtonsToPress.size(); ++b )
		{
			const InputEventPlus *pIEP = NULL;
			int iQueueSearchIndex = iQueueIndex;
			for( ; iQueueSearchIndex>=0; --iQueueSearchIndex )	
			{
				const InputEventPlus &iep = aQueue[iQueueSearchIndex];
				if( iep.DeviceI.ts < OldestTimeAllowedForTap )
					break;

				if( !Press.m_InputTypes[iep.type] )
					continue;

				if( iep.GameI.button == Press.m_aButtonsToPress[b] )
				{
					pIEP = &iep;
					break;
				}
			}
			if( pIEP == NULL )
				break;	

			bool bAllHeldButtonsOK = true;
			for( unsigned i=0; i<Press.m_aButtonsToHold.size(); i++ )
			{
				GameInput gi( controller, Press.m_aButtonsToHold[i] );
				if( !INPUTMAPPER->IsBeingPressed(gi, MultiPlayer_Invalid, &pIEP->InputList) )
					bAllHeldButtonsOK = false;
			}
			for( unsigned i=0; i<Press.m_aButtonsToNotHold.size(); i++ )
			{
				GameInput gi( controller, Press.m_aButtonsToNotHold[i] );
				if( INPUTMAPPER->IsBeingPressed(gi, MultiPlayer_Invalid, &pIEP->InputList) )
					bAllHeldButtonsOK = false;
			}
			if( !bAllHeldButtonsOK )
				continue;
			iMinSearchIndexUsed = min( iMinSearchIndexUsed, iQueueSearchIndex );
			if( b == (int) Press.m_aButtonsToPress.size()-1 )
				bMatched = true;
		}

		if( !bMatched )
		{
			if( !Press.m_bAllowIntermediatePresses )
				return false;
			--iQueueIndex;
			continue;
		}

		if( iSequenceIndex == 0 )
		{
			INPUTQUEUE->ClearQueue( controller );
			return true;
		}

		iQueueIndex = iMinSearchIndexUsed - 1;
		--iSequenceIndex;
	}

	return false;
}

bool InputQueueCode::Load( RString sButtonsNames )
{
	m_aPresses.clear();

	vector<RString> asPresses;
	split( sButtonsNames, ",", asPresses, false );
	FOREACH( RString, asPresses, sPress )
	{
		vector<RString> asButtonNames;

		split( *sPress, "-", asButtonNames, false );

		if( asButtonNames.size() < 1 )
		{
			if( sButtonsNames != "" )
				LOG->Trace( "Ignoring empty code \"%s\".", sButtonsNames.c_str() );
			return false;
		}

		m_aPresses.push_back( ButtonPress() );
		for( unsigned i=0; i<asButtonNames.size(); i++ )	
		{
			RString sButtonName = asButtonNames[i];

			bool bHold = false;
			bool bNotHold = false;
			while(1)
			{
				if( sButtonName.Left(1) == "+" )
				{
					m_aPresses.back().m_InputTypes[IET_REPEAT] = true;
					sButtonName.erase(0, 1);
				}
				else if (sButtonName.Left(1) == "~")
				{
					m_aPresses.back().m_InputTypes[IET_FIRST_PRESS] = false;
					m_aPresses.back().m_InputTypes[IET_RELEASE] = true;
					sButtonName.erase(0, 1);
				}
				else if (sButtonName.Left(1) == "@")
				{
					sButtonName.erase(0, 1);
					bHold = true;
				}
				else if (sButtonName.Left(1) == "!")
				{
					sButtonName.erase(0, 1);
					bNotHold = true;
				}
				else
				{
					break;
				}
			}

			const GameButton gb = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex(sButtonName);
			if( gb == GameButton_Invalid )
			{
				LOG->Trace( "The code \"%s\" contains an unrecognized button \"%s\".", sButtonsNames.c_str(), sButtonName.c_str() );
				m_aPresses.clear();
				return false;
			}

			if (bHold)
				m_aPresses.back().m_aButtonsToHold.push_back(gb);
			else if (bNotHold)
				m_aPresses.back().m_aButtonsToNotHold.push_back(gb);
			else
				m_aPresses.back().m_aButtonsToPress.push_back(gb);
		}
	}
	
	if (m_aPresses.size() == 1)
		m_fMaxSecondsBack = 0.55f;
	else
		m_fMaxSecondsBack = (m_aPresses.size() - 1) * 0.6f;

	return true;
}


