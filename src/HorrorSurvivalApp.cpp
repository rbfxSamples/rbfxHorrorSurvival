#include "HorrorSurvivalApp.h"

#include <Urho3D/UI/SplashScreen.h>
#include <Urho3D/Engine/EngineDefs.h>

#include "GameScreen.h"
#include "Urho3D/IO/IOEvents.h"

using namespace Urho3D;

HorrorSurvivalApp::HorrorSurvivalApp(Urho3D::Context* context) :
	PluginApplication(context)
{
}

//void HorrorSurvivalApp::Setup()
//{
//#if defined(DEBUG)
//	engineParameters_[EP_FULL_SCREEN] = false;
//#endif
//}

void HorrorSurvivalApp::Load()
{
	SubscribeToEvent(E_LOGMESSAGE, URHO3D_HANDLER(HorrorSurvivalApp, HandleLogMessage));
}

void  HorrorSurvivalApp::HandleLogMessage(Urho3D::StringHash eventType, Urho3D::VariantMap& args)
{
	using namespace LogMessage;

	auto logMessage = args[P_LEVEL].GetInt();
	if (logMessage == LOG_ERROR)
	{
		auto msg = args[P_MESSAGE].GetString();
	}
}

void HorrorSurvivalApp::Start()
{
	auto* gameScreen = new GameScreen(context_);
	context_->GetSubsystem<StateManager>()->EnqueueState(gameScreen);
}

void HorrorSurvivalApp::Stop()
{
}

void HorrorSurvivalApp::Unload()
{
	context_->GetSubsystem<StateManager>()->Reset();
}
