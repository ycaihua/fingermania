#include "global.h"
#include "CodeDetector.h"
#include "PlayerOptions.h"
#include "GameState.h"
#include "InputQueue.h"
#include "ThemeManager.h"
#include "RageLog.h"
#include "Game.h"
#include "RageUtil.h"
#include "PlayerState.h"
#include "InputEventPlus.h"

const char* CodeNames[] = 
{
	"PrevSteps1",
	"PrevSteps2",
	"NextSteps1",
	"NextSteps2",
	"NextSort1",
	"NextSort2",
	"NextSort3",
	"NextSort4",
	"ModeMenu1",
	"ModeMenu2",
	"Mirror",
	"Left",
	"Right",
	"Shuffle",
	"superShuffle",
	"NextTransform",
	"NextScrollSpeed",
	"PreviousScrollSpeed",
	"NextAccel",
	"NextEffect",
	"NextAppearance",
	"NextTurn",
	"Reverse",
	"HoldNotes",
	"Mines",
	"Dark",
	"Hidden",
	"RandomVanish",
	"CancelAll",
	"NextTheme",
	"NextTheme2",
	"NextAnnouncer",
	"NextAnnouncer2",
	"NextBannerGroup",
	"NextBannerGroup2",
	"SaveScreenshot1",
	"SaveScreenshot2",
	"CancelAllPlayerOptions",
	"BackInEventMode",
};
XToString(Code);

static InputQueueCode g_CodeItems[NUM_Code];

bool CodeDetector::EnteredCode(GameController controller, Code code)
{
	return g_CodeItems[code].EnteredCode(controller);
}

void CodeDetector::RefreshCacheItems(RString sClass)
{
	if (sClass == "")
		sClass = "CodeDetector";
	FOREACH_ENUM(Code, r)
	{
		InputQueueCode& item = g_CodeItems[c];
		const RString sCodeName = CodeToString(c);
		const RString sButtonsNames = THEME->GetMetric(sClass, sCodeName);

		item.Load(sButtonsNames);
	}
}

bool CodeDetector::EnteredNextBannerGroup(GameController controller)
{
	return EnteredCode(controller, CODE_BW_NEXT_GROUP) || EnteredCode(controller, CODE_BW_NEXT_GROUP2);
}

bool CodeDetector::EnteredPrevSteps(GameController controller)
{
	return EnteredCode(controller, Code_PrevSteps1) || EnteredCode(controller, Code_PrevStep2);
}

bool CodeDetector::EnteredNextSteps(GameController controller)
{
	return EnteredCode(controller, Code_NextSteps1) || EnteredCode(controller, Code_NextSteps2);
}

bool CodeDetector::EnteredNextSort(GameController controller)
{
	return EnteredCode(controller, CODE_NEXT_SORT1) ||
		   EnteredCode(controller, CODE_NEXT_SORT2) ||
		   EnteredCode(controller, CODE_NEXT_SORT3) ||
		   EnteredCode(controller, CODE_NEXT_SORT4);
}

bool CodeDetector::EnteredModeMenu(GameController controller)
{
	return EnteredCode(controller, CODE_MODE_MENU1) || EnteredCode(controller, CODE_MODE_MENU2);
}

#define TOGGLE(v, a, b) if (v != a) v = a; else v = b;
#define FLOAT_TOGGLE(v) if (v != 1.f) v = 1.f; else v = 0.f;
#define INCREMENT_SCROLL_SPEED(s) (s == 0.5f) ? s = 0.75f : (s == 0.75f) ? s = 1.0f : (s == 1.0f) ? s = 1.5f : (s == 1.5f) ? s = 2.0f : (s == 2.0f) ? s = 3.0f : (s == 3.0f) ? s = 4.0f : (s == 4.0f) ? s = 5.0f : (s == 5.0f) ? s = 8.0f : s = 0.5f;
#define DECREMENT_SCROLL_SPEED(s) (s == 0.75f) ? s = 0.5f : (s == 1.0f) ? s = 0.75f : (s == 1.5f) ? s = 1.0f : (s == 2.0f) ? s = 1.5f : (s == 3.0f) ? s = 2.0f : (s == 4.0f) ? s = 3.0f : (s == 5.0f) ? s = 4.0f : (s == 8.0f) ? s = 4.0f : s = 8.0f;

bool CodeDetector::DetectAndAdjustMusicOptions(GameController controller)
{
	PlayerNumber pn = INPUTMAPPER->ControllerToPlayerNumber(controller);

	for (int c = CODE_MIRROR; c <= CODE_CANCEL_ALL; c++)
	{
		Code code = (Code)c;

		PlayerOptions po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetPreferred();

		if (EnteredCode(controller, code))
		{
			swtich(code)
			{
				case CODE_MIRROR:
					po.ToggleOneTurn(PlayerOptions::TURN_MIRROR); break;
				case CODE_LEFT:
					po.ToggleOneTurn(PlayerOptions::TURN_LEFT); break;
				case CODE_RIGHT:
					po.ToggleOneTurn(PlayerOptions::TURN_RIGHT); break;
				case CODE_SHUFFLE:
					po.ToggleOneTurn(PlayerOptions::TURN_SHUFFLE); break;
				case CODE_SUPER_SHUFFLE:
					po.ToggleOneTurn(PlayerOptions::TURN_SUPER_SHUFFLE); break;
				case CODE_NEXT_TRANSFORM:
					po.NextTransform(); break;
				case CODE_NEXT_SCROLL_SPEED:
					INCREMENT_SCROLL_SPEED(po.m_fScrollSpeed);
					break;
				case CODE_PREVIOUS_SCROLL_SPEED:
					DECREMENT_SCROLL_SPEED(po.m_fScrollSpeed);
					break;
				case CODE_NEXT_ACCEL:
					po.NextAccel();
					break;
				case CODE_NEXT_EFFECT:
					po.NextEffect();
					break;
				case CODE_NEXT_APPEARANCE:
					po.NextAppearance();
					break;
				case CODE_NEXT_TURN:
					po.NextTurn();
					break;
				case CODE_REVERSE:
					po.NextScroll();
					break;
				case CODE_HOLDS:
					TOGGLE(po.m_bTransforms[PlayerOptions::TRANSFORM_NOHOLDS], true, false); 
					break;
				case CODE_MINES:
					TOGGLE(po.m_bTransforms[PlayerOptions::TRANSFORM_NOMINES], true, false);
					break;
				case CODE_DARK:
					FLOAT_TOGGLE(po.m_fDark);
					break;
				case CODE_CANCEL_ALL:
					GAMESTATE->GetDefaultPlayerOptions(po);
					break;
				case CODE_HIDDEN:
					ZERO(po.m_fAppearances);
					po.m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN] = 1;
					break;
				case CODE_RANDOMVANISH:
					ZERO(po.m_fAppearances);
					po.m_fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] = 1;
					break;
				default:
					break;
			}

			GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.Assign(ModsLevel_Preferred, po);

			return true;
		}
	}

	return false;
}
