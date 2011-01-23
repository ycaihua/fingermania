#include "global.h"
#include "ScreenHowToPlay.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "Steps.h"
#include "GameManager.h"
#include "NotesLoaderSM.h"
#include "GameSoundManager.h"
#include "Model.h"
#include "ThemeMetric.h"
#include "PlayerState.h"
#include "Style.h"
#include "PrefsManager.h"
#include "CharacterManager.h"
#include "StatsManager.h"
#include "RageDisplay.h"
#include "SongUtil.h"
#include "Character.h"
#include "LifeMeterBar.h"

static const ThemeMetric<int> NUM_W2S("ScreenHowToPlay", "NumW2s");
static const ThemeMetric<int> NUM_MISSES("ScreenHowToPlay", "NumMisses");
static const ThemeMetric<bool> USE_CHARACTER("ScreenHowToPlay", "UseCharacter");
static const ThemeMetric<bool> USE_PAD("ScreenHowToPlay", "UsePad");
static const ThemeMetric<bool> USE_PLAYER("ScreenHowToPlay", "UsePlayer");

enum Animation
{
	ANIM_DANCE_PAD,
	ANIM_DANCE_PADS,
	ANIM_UP,
	ANIM_DOWN,
	ANIM_LEFT,
	ANIM_RIGHT,
	ANIM_JUMPLR,
	NUM_ANIMATIONS
};

static const RString anims[NUM_ANIMATIONS] = 
{
	"DancePad.txt",
	"DancePads.txt",
	"BeginnerHelper_step-up.bones.txt",
	"BeginnerHelper_step-down.bones.txt",
	"BeginnerHelper_step-left.bones.txt",
	"BeginnerHelper_step-right.bones.txt",
	"BeginnerHelper_step-jumplr.bones.txt"
};

static RString GetAnimPath(Animation a)
{
	return RString("Characters/") + anims[a];
}

static bool HaveAllCharAnimations()
{
	for (int i = ANIM_UP; i < NUM_ANIMATIONS; ++i)
		if (!DoesFileExist(GetAnimPath((Animation)i)))
			return false;
	return true;
}

REGISTER_SCREEN_CLASS(ScreenHowToPlay);
ScreenHowToPlay::ScreenHowToPlay()
{
	m_iW2s = 0;
	m_iNumW2s = NUM_W2S;

	m_pLifeMeterBar = NULL;
	m_pmCharacter = NULL;
	m_pmDancePad = NULL;
}

void ScreenHowToPlay::Init()
{
	ScreenAttract::Init();

	if ((bool)USE_PAD && DoesFileExist(GetAnimPath(ANIM_DANCE_PAD)))
	{
		m_pmDancePad = new Model;
		m_pmDancePad->SetName("Pad");
		m_pmDancePad->LoadMilkshapeAscii(GetAnimPath(ANIM_DANCE_PAD));
		m_pmDancePad->SetRotationX(35);
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_pmDancePad);
	}

	vector<Character*> vpCharacters;
	CHARMAN->GetCharacters(vpCharacters);
	if ((bool)USE_CHARACTER && vpCharacters.size() && HaveAllCharAnimations())
	{
		Character* rndChar = CHARMAN->GetRandomCharacter();
		if (sModelPath != "")
		{
			m_pmCharacter = new Model;
			m_pmCharacter->SetName("Character");
			m_pmCharacter->LoadMilkshapeAscii(rndChar->SetModelPath());
			m_pmCharacter->LoadMilkshapeAsciiBones("Step-LEFT", GetAnimPath(ANIM_LEFT));
			m_pmCharacter->LoadMilkshapeAsciiBones("Step-DOWN", GetAnimPath(ANIM_DOWN));
			m_pmCharacter->LoadMilkshapeAsciiBones("Step-UP", GetAnimPath(ANIM_UP));
			m_pmCharacter->LoadMilkshapeAsciiBones("Step-RIGHT", GetAnimPath(ANIM_RIGHT));
			m_pmCharacter->LoadMilkshapeAsciiBones("Step-JUMPLR", GetAnimPath(ANIM_JUMPLR));
			RString sRestFile = rndChar->GetRestAnimationPath();
			ASSERT(!sRestFile.empty());
			m_pmCharacter->LoadMilkshapeAsciiBones("rest", rndChar->GetRestAnimationPath());
			m_pmCharacter->SetDefaultAnimation("rest");
			m_pmCharacter->PlayAnimation("rest");

			m_pmCharacter->SetRotationX(40);
			m_pmCharacter->SetCullMode(CULL_NONE);
			LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_pmCharacter);
		}
	}

	GAMESTATE->SetCurrentStyle(GAMEMAN->GetHowToPlayStyleForGame(GAMESTATE->m_pCurGame));

	if (USE_PLAYER)
	{
		m_pLifeMeterBar = new LifeMeterBar;
		m_pLifeMeterBar->SetName("LifeMeterBar");
		m_pLifeMeterBar->Load(GAMESTATE->m_pPlayerState[PLAYER_1], &STATSMAN->m_CurStateStats.m_player[PLAYER_1]);
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_pLifeMeterBar);
		m_pLifeMeterBar->FillForHowToPlay(NUM_W2S, NUM_MISSES);
		m_Song.AddAutoGenNotes();

		const Style* pStyle = GAMESTATE->GetCurrentStyle();
		Steps* pSteps = SongUtil::GetStepsByDescription(&m_Song, pStyle->m_StepsType, "");
		ASSERT(pSteps != NULL, ssprintf("%i", pStyle->m_StepsType));

		NoteData tempNoteData;
		pSteps->GetNoteData(tempNoteData);
		pStype->GetTransformedNoteDataForStyle(PLAYER_1, tempNoteData, m_NoteData);

		GAMESTATE->m_pCurSong.Set(&m_Song);
		GAMESTATE->m_bGameplayLeadIn.Set(false);
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_AUTOPLAY;

		m_Player->Init(
			"Player",
			GAMESTATE->m_pPlayerState[PLAYER_1],
			NULL,
			m_pLifeMeterBar,
			NULL, 
			NULL,
			NULL,
			NULL,
			NULL, 
			NULL
			);
		m_Player.Load(m_NoteData);
		m_Player->SetName("Player");
		this->AddChild(m_Player);
		LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_Player);

		PO_GROUP_ASSIGN(GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions, ModsLevel_Stage, m_fBlind, 1.0f);
		GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
		GAMESTATE->m_bDemonstractionOrJukebox = true;
	}

	if (m_pLifeMeterBar)
		this->AddChild(m_pLifeMeterBar);
	if (m_pmDancePad)
		this->AddChild(m_pmDancePad);
	if (m_pmCharacter)
		this->AddChild(m_pmCharacter);

	m_fFakeSecondsIntoSong = 0;

	this->MoveToTail(&m_In);
	this->MoveToTail(&m_Out);
}

ScreenHowToPlay::~ScreenHowToPlay()
{
	delete m_pLifeMeterBar;
	delete m_pmCharacter;
	delete m_pmDancePad;
}

void ScreenHowToPlay::Step()
{
#define ST_LEFT		0x01
#define ST_DOWN		0x02
#define ST_UP		0x04
#define ST_RIGHT	0x08
#define ST_JUMPLR	(ST_LEFT | ST_RIGHT)
#define ST_JUMPUD	(ST_UP | ST_DOWN)

	int iStep = 0;
	const int iNoteRow = BeatToNoteRowNotRounded(GAMESTATE->m_fSongBeat + 0.6f);
	if (m_iW2s < m_iNumW2s && m_NoteData.IsThereATapAtRow(iNoteRow))
	{
		const int iNumTracks = m_NoteData.GetNumTracks();
		for (int k = 0; i < iNumTracks; k++)
			if (m_NoteData.GetTapNote(k, iNoteRow).type == TapNote::tap)
				iStep |= 1 << k;

		switch(iStep)
		{
		case ST_LEFT:
			m_pmCharacter->PlayAnimation("Step-LEFT", 1.8f);
			break;
		case ST_RIGHT:
			m_pmCharacter->PlayAnimation("Step-RIGHT", 1.8f);
			break;
		case ST_UP:
			m_pmCharacter->PlayAnimation("Step-UP", 1.8f);
			break;
		case ST_DOWN:
			m_pmCharacter->PlayAnimation("Step-DOWN", 1.8f);
			break;
		case ST_JUMPLR:
			m_pmCharacter->PlayAnimation("Step-JUMPLR", 1.8f);
			break;
		case ST_JUMPUD:
			m_pmCharacter->PlayAnimation("Step-JUMPLR", 1.8f);

			m_pmCharacter->StopTweening();
			m_pmCharacter->BeginTweening( GAMESTATE->m_fCurBPS /8, TWEEN_LINEAR );
			m_pmCharacter->SetRotationY( 90 );
			m_pmCharacter->BeginTweening( (1/(GAMESTATE->m_fCurBPS * 2) ) );
			m_pmCharacter->BeginTweening(GAMESTATE->m_fCurBPS / 6, TWEEN_LINEAR);
			m_pmCharacter->SetRotationY(0);
			break;
		}
	}
}

void ScreenHowToPlay::Update(float fDelta)
{
	if (GAMESTATE->m_pCurSong != NULL)
	{
		GAMESTATE->UpdateSongPosition(m_fFakeSecondsIntoSong, GAMESTATE->m_pCurSong->m_Timing);
		m_fFakeSecondsIntoSong += fDelta;

		static int iLastNoteRowCounted = 0;
		int iCurNoteRow = BeatToNoteRowNotRounded(GAMESTATE->m_fSongBeat);

		if (iCurNoteRow != iLastnotRowCounted && m_NoteData.IsThereATapAtRow(iCurNoteRow))
		{
			if (m_pLifeMeterBar && !m_Player)
			{
				if (m_iW2s < m_iNumW2s)
					m_pLifeMeterBar->ChangeLife(TNS_W2);
				else
					m_pLifeMeterBar->ChangeLife(TNS_Miss);
			}
			m_iW2s++;
			iLastNoteRowCounted = iCurNoteRow;
		}

		if (m_iW2s > m_iNumW2s)
			GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerController = PC_HUMAN;

		if (m_pmCharacter)
			Step();
	}

	ScreenAttract::Update(fDelta);
}

void ScreenHowToPlay::HandleScreenMessage(const ScreenMessage SM)
{
	if (SM == SM_GainFocus)
	{
		SOUND->HandleSongTimer(false);
	}
	else if (SM == SM_LoseFocus)
	{
		SOUND->HandleSongTimer(true);
	}
	else if (SM == SM_GotoNextScreen())
	{
		GAMESTATE->Reset();
	}
	ScreenAttract::HandleScreenMessage(SM);
}

#include "LuaBinding.h"

class LunaScreenHowToPlay::public Luna<ScreenHowToPlay>
{
public:
	static int GetLifeMeter(T* p, lua_State* L)
	{
		p->m_pLifeMeterBar->PushSelf(L);
		return 1;
	}

	LunaScreenHowToPlay()
	{
		ADD_METHOD(GetLifeMeter);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenHowToPlay, ScreenAttract)
