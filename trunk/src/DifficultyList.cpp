#include "global.h"
#include "DifficultyList.h"
#include "GameState.h"
#include "Song.h"
#include "Steps.h"
#include "Style.h"
#include "StepsDisplay.h"
#include "StepsUtil.h"
#include "CommonMetrics.h"
#include "Foreach.h"
#include "SongUtil.h"
#include "XmlFile.h"

#define MAX_METERS NUM_Difficulty + MAX_EDITS_PER_SONG

StepsDisplayList::StepsDisplayList()
{
	m_bShown = true;

	FOREACH_ENUM(PlayerNumber, pn)
	{
		SubscribeToMessage((MessageID)(Message_CurrentStepsP1Changed + pn));
		SubscribeToMessage((MessageID)(Message_CurrentTrailP1Changed + pn));
	}
}

StepsDisplayList::~StepsDisplayList()
{
}

void StepsDisplayList::LoadFromNode(const XNode* pNode)
{
	ActorFrame::LoadFromNode(pNode);

	ASSERT_M(!m_sName.empty(), "StepsDisplayList must have a Name");

	ITEMS_SPACING_Y.Load(m_sName, "ItemsSpacing");
	NUM_SHOWN_ITEMS.Load(m_sName, "NumShownItems");
	CAPITALIZE_DIFFICULTY_NAMES.Load(m_sName, "CapitalizeDifficultyNames");
	MOVE_COMMAND.Load(m_sName, "MoveCommand");

	m_Lines.resize(MAX_METERS);
	m_CurSong = NULL;

	FOREACH_ENUM(PlayerNumber, pn)
	{
		const XNode* pChild = pNode->GetChild(ssprintf("CursorP%d", pn + 1));
		if (pChild == NULL)
			RageException::Throw("%s: StepsDisplayList: missing the node \"CursorP%d\"", ActorUtil::GetWhere(pNode).c_str(), pn + 1);
		m_Cursors[pn].LoadActorFromNode(pChild, this);

		pChild = pNode->GetChild(ssprintf("CursorP%iFrame", pn + 1));
		if (pChild == NULL)
			RageException::Throw("%s: StepsDisplayList: missing the node \"CursorP%dFrame\"", ActorUtil::GetWhere(pNode).c_str(), pn + 1);
		m_CursorFrames[pn].LoadFromNode(pChild);
		m_CursorFrames[pn].AddChild(m_Cursors[pn]);
		this->AddChild(&m_CursorFrames[pn]);
	}

	for (unsigned m = 0; m < m_Lines.size(); ++m)
	{
		m_Lines[m].m_Meter.SetName("Row");
		m_Lines[m].m_Meter.Load("StepsDisplayListRow", NULL);
		this->AddChild(&m_Lines[m].m_Meter);
	}

	UpdatePositions();
	PositionItems();
}

int StepsDisplayList::GetCurrentRowIndex(PlayerNumber pn) const
{
	Difficulty ClosestDifficulty = GAMESTATE->GetClosestShownDifficulty(pn);

	for (unsigned i = 0; i < m_Rows.size(); i++)
	{
		consr Row& row = m_Rows[i];

		if (GAMESTATE->m_pCurSteps[pn] == NULL)
		{
			if (row.m_dc == ClosestDifficulty)
				return i;
		}
		else
		{
			if (GAMESTATE->m_pCurSteps[pn].Get() == row.m_Steps)
				return i;
		}
	}

	return 0;
}

void StepsDisplayList::UpdatePositions()
{
	int iCurrentRow[NUM_PLAYERS];
	FOREACH_HumanPlayer(p)
		iCurrentRow[p] = GetCurrentRowIndex(p);

	const int total = NUM_SHOWN_ITEMS;
	const int halfsize = total / 2;

	int first_start, first_end, second_start, second_end;

	int P1Choice = GAMESTATE->IsHumanPlayer(PLAYER_1)? iCurrentRow[PLAYER_1]: GAMESTATE->IsHumanPlayer(PLAYER_2)? iCurrentRow[PLAYER_2]: 0;
	int P2Choice = GAMESTATE->IsHumanPlayer(PLAYER_2)? iCurrentRow[PLAYER_2]: GAMESTATE->IsHumanPlayer(PLAYER_1)? iCurrentRow[PLAYER_1]: 0;

	vector<Row> &Rows = m_Rows;

	const bool BothPlayersActivated = GAMESTATE->IsHumanPlayer(PLAYER_1) && GAMESTATE->IsHumanPlayer(PLAYER_2);
	if( !BothPlayersActivated )
	{
		first_start = max( P1Choice - halfsize, 0 );
		first_end = first_start + total;
		second_start = second_end = first_end;
	}
	else
	{
		const int earliest = min( P1Choice, P2Choice );
		first_start = max( earliest - halfsize/2, 0 );
		first_end = first_start + halfsize;

		const int latest = max( P1Choice, P2Choice );

		second_start = max( latest - halfsize/2, 0 );

		second_start = max( second_start, first_end );

		second_end = second_start + halfsize;
	}

	first_end = min( first_end, (int) Rows.size() );
	second_end = min( second_end, (int) Rows.size() );

	while(1)
	{
		const int sum = (first_end - first_start) + (second_end - second_start);
		if( sum >= (int) Rows.size() || sum >= total)
			break;

		if( second_start > first_end )
			second_start--;

		else if( first_start > 0 )
			first_start--;
		else if( second_end < (int) Rows.size() )
			second_end++;
		else
			ASSERT(0);
	}

	int pos = 0;
	for( int i=0; i<(int) Rows.size(); i++ )	
	{
		float ItemPosition;
		if( i < first_start )
			ItemPosition = -0.5f;
		else if( i < first_end )
			ItemPosition = (float) pos++;
		else if( i < second_start )
			ItemPosition = halfsize - 0.5f;
		else if( i < second_end )
			ItemPosition = (float) pos++;
		else
			ItemPosition = (float) total - 0.5f;
			
		Row &row = Rows[i];

		float fY = ITEMS_SPACING_Y*ItemPosition;
		row.m_fY = fY;
		row.m_bHidden = i < first_start ||
							(i >= first_end && i < second_start) ||
							i >= second_end;
	}
}

void StepsDisplayList::PositionItems()
{
	for( int i = 0; i < MAX_METERS; ++i )
	{
		bool bUnused = ( i >= (int)m_Rows.size() );
		m_Lines[i].m_Meter.SetVisible( !bUnused );
	}

	for( int m = 0; m < (int)m_Rows.size(); ++m )
	{
		Row &row = m_Rows[m];
		bool bHidden = row.m_bHidden;
		if( !m_bShown )
			bHidden = true;

		const float fDiffuseAlpha = bHidden? 0.0f:1.0f;
		if( m_Lines[m].m_Meter.GetDestY() != row.m_fY ||
			m_Lines[m].m_Meter.DestTweenState().diffuse[0][3] != fDiffuseAlpha )
		{
			m_Lines[m].m_Meter.RunCommands( MOVE_COMMAND.GetValue() );
			m_Lines[m].m_Meter.RunCommandsOnChildren( MOVE_COMMAND.GetValue() );
		}

		m_Lines[m].m_Meter.SetY( row.m_fY );
	}

	for( int m=0; m < MAX_METERS; ++m )
	{
		bool bHidden = true;
		if( m_bShown && m < (int)m_Rows.size() )
			bHidden = m_Rows[m].m_bHidden;

		float fDiffuseAlpha = bHidden?0.0f:1.0f;

		m_Lines[m].m_Meter.SetDiffuseAlpha( fDiffuseAlpha );
	}


	FOREACH_HumanPlayer( pn )
	{
		int iCurrentRow = GetCurrentRowIndex( pn );

		float fY = 0;
		if( iCurrentRow < (int) m_Rows.size() )
			fY = m_Rows[iCurrentRow].m_fY;

		m_CursorFrames[pn].PlayCommand( "Change" );
		m_CursorFrames[pn].SetY( fY );
	}
}

void StepsDisplayList::SetFromGameState()
{
	const Song *pSong = GAMESTATE->m_pCurSong;
	unsigned i = 0;
	
	if( pSong == NULL )
	{
		const vector<Difficulty>& difficulties = CommonMetrics::DIFFICULTIES_TO_SHOW.GetValue();
		m_Rows.resize( difficulties.size() );
		FOREACH_CONST( Difficulty, difficulties, d )
		{
			m_Rows[i].m_dc = *d;
			m_Lines[i].m_Meter.SetFromStepsTypeAndMeterAndDifficultyAndCourseType( GAMESTATE->m_pCurStyle->m_StepsType, 0, *d, CourseType_Invalid );
			++i;
		}
	}
	else
	{
		vector<Steps*>	vpSteps;
		SongUtil::GetPlayableSteps( pSong, vpSteps );
		
		m_Rows.resize( vpSteps.size() );
		FOREACH_CONST( Steps*, vpSteps, s )
		{
			m_Rows[i].m_Steps = *s;
			m_Lines[i].m_Meter.SetFromSteps( *s );
			++i;
		}
	}
	while( i < MAX_METERS )
		m_Lines[i++].m_Meter.Unset();

	UpdatePositions();
	PositionItems();

	for( int m = 0; m < MAX_METERS; ++m )
		m_Lines[m].m_Meter.FinishTweening();
}

void StepsDisplayList::HideRows()
{
	for( unsigned m = 0; m < m_Rows.size(); ++m )
	{
		Line &l = m_Lines[m];

		l.m_Meter.FinishTweening();
		l.m_Meter.SetDiffuseAlpha(0);
	}
}

void StepsDisplayList::TweenOnScreen()
{
	FOREACH_HumanPlayer( pn )
		ON_COMMAND( m_Cursors[pn] );

	for( int m = 0; m < MAX_METERS; ++m )
		ON_COMMAND( m_Lines[m].m_Meter );

	this->SetHibernate( 0.5f );
	m_bShown = true;
	for( unsigned m = 0; m < m_Rows.size(); ++m )
	{
		Line &l = m_Lines[m];

		l.m_Meter.FinishTweening();
	}

	HideRows();
	PositionItems();

	FOREACH_HumanPlayer( pn )
		COMMAND( m_Cursors[pn], "TweenOn" );
}

void StepsDisplayList::TweenOffScreen()
{

}

void StepsDisplayList::Show()
{
	m_bShown = true;

	SetFromGameState();

	HideRows();
	PositionItems();

	FOREACH_HumanPlayer( pn )
		COMMAND( m_Cursors[pn], "Show" );
}

void StepsDisplayList::Hide()
{
	m_bShown = false;
	PositionItems();

	FOREACH_HumanPlayer( pn )
		COMMAND( m_Cursors[pn], "Hide" );
}

void StepsDisplayList::HandleMessage( const Message &msg )
{
	FOREACH_ENUM( PlayerNumber, pn )
	{
		if( msg.GetName() == MessageIDToString((MessageID)(Message_CurrentStepsP1Changed+pn))  ||
			msg.GetName() == MessageIDToString((MessageID)(Message_CurrentTrailP1Changed+pn)) ) 
		SetFromGameState();
	}

	ActorFrame::HandleMessage(msg);
}

#include "LuaBinding.h"

class LunaStepsDisplayList: public Luna<StepsDisplayList>
{
public:
	static int setfromgamestate( T* p, lua_State *L )		{ p->SetFromGameState(); return 0; }

	LunaStepsDisplayList()
	{
		ADD_METHOD( setfromgamestate );
	}
};

LUA_REGISTER_DERIVED_CLASS( StepsDisplayList, ActorFrame )

