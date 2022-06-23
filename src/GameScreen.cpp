#include "GameScreen.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Input/FreeFlyController.h>

using namespace Urho3D;

GameScreen::GameScreen(Urho3D::Context* context) :
	ApplicationState(context)
{
	scene_ = new Scene(context_);
	scene_->LoadFile("Scenes/Scene.xml");

	Camera* camera = scene_->GetComponent<Camera>(true);

	SharedPtr<Viewport> viewport{ new Viewport(context_, scene_, camera) };
	SetViewport(0, viewport);

	auto* cameraController = camera->GetNode()->CreateComponent<FreeFlyController>();
	cameraController->SetSpeed(2.0f);
	cameraController->SetAcceleratedSpeed(20.0f);

	// Set the mouse mode to use in the sample
	SetMouseMode(MM_FREE);
	SetMouseVisible(true);
}

void GameScreen::Activate(Urho3D::SingleStateApplication* application)
{
	ApplicationState::Activate(application);
}

void GameScreen::Deactivate()
{
	ApplicationState::Deactivate();
}
