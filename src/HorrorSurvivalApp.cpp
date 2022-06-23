#include "HorrorSurvivalApp.h"

#include <Urho3D/UI/SplashScreen.h>
#include <Urho3D/Engine/EngineDefs.h>

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
	auto* splash = new SplashScreen(context_);
	splash->SetDefaultFogColor(Color::BLUE);
	SetState(splash);
}

void HorrorSurvivalApp::Stop()
{
	
}