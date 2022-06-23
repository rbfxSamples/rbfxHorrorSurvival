#include "HorrorSurvivalApp.h"

#include <Urho3D/UI/SplashScreen.h>
#include <Urho3D/Engine/EngineDefs.h>

#include "GameScreen.h"

using namespace Urho3D;

HorrorSurvivalApp::HorrorSurvivalApp(Urho3D::Context* context) :
	SingleStateApplication(context)
{
}

void HorrorSurvivalApp::Setup()
{
#if defined(DEBUG)
	engineParameters_[EP_FULL_SCREEN] = false;
#endif
}

void HorrorSurvivalApp::Start()
{
	auto* gameScreen = new GameScreen(context_);
	SetState(gameScreen);
}

void HorrorSurvivalApp::Stop()
{
	
}