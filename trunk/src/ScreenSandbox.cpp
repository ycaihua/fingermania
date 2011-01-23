#include "global.h"
#include "ScreenSandbox.h"
#include "ThemeManager.h"
#include "RageDisplay.h"
#include "RageLog.h"

REGISTER_SCREEN_CLASS(ScreenSandbox);

void ScreenSandbox::HandleScreenMessage(const ScreenMessage SM)
{
	Screen::HandleScreenMessage(SM);
}

void ScreenSandbox::Input(const InputEventPlus& input)
{
	Screen::Input(input);
}

void ScreenSandbox::Update(float fDeltaTime)
{
	Screen::Update(fDeltaTime);
}

void ScreenSandbox::DrawPrimitives()
{
	Screen::DrawPrimitives();
}
