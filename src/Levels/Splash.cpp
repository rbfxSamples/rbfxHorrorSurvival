#include <Urho3D/UI/UI.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/Context.h>
#include "Splash.h"
#include "../Messages/Achievements.h"
#include "../LevelManagerEvents.h"

using namespace Levels;
using namespace LevelManagerEvents;

static int SPLASH_TIME = 3000;

Splash::Splash(Context* context) :
    BaseLevel(context),
    logoIndex_(0)
{
    // List of different logos that multiple splash screens will show
    logos_.Reserve(1);
    logos_.Push("Textures/UrhoIcon.png");
//    logos_.Push("Textures/Achievements/retro-controller.png");
}

Splash::~Splash()
{
}

void Splash::RegisterObject(Context* context)
{
    context->RegisterFactory<Splash>();
}

void Splash::Init()
{
    // Disable achievement showing for this level
    GetSubsystem<Achievements>()->SetShowAchievements(false);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Subscribe to global events for camera movement
    SubscribeToEvents();
}

void Splash::CreateScene()
{
    return;
}

void Splash::CreateUI()
{
    timer_.Reset();
    UI* ui = GetSubsystem<UI>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Get the current logo index
    logoIndex_ = data_["LogoIndex"].GetInt();

    // Get the Urho3D fish texture
    auto* decalTex = cache->GetResource<Texture2D>(logos_[logoIndex_]);
    // Create a new sprite, set it to use the texture
    SharedPtr<Sprite> sprite(new Sprite(context_));
    sprite->SetTexture(decalTex);

    auto* graphics = GetSubsystem<Graphics>();

    // Get rendering window size as floats
    auto width = (float)graphics->GetWidth() / GetSubsystem<UI>()->GetScale();
    auto height = (float)graphics->GetHeight() / GetSubsystem<UI>()->GetScale();

    // The UI root element is as big as the rendering window, set random position within it
    sprite->SetPosition(width / 2, height / 2);

    // Avoid having too large logos
    // We assume here that the logo image is a regular rectangle
    if (decalTex->GetWidth() <= 256 && decalTex->GetHeight() <= 256) {
        // Set sprite size & hotspot in its center
        sprite->SetSize(IntVector2(decalTex->GetWidth(), decalTex->GetHeight()));
        sprite->SetHotSpot(IntVector2(decalTex->GetWidth() / 2, decalTex->GetHeight() / 2));
    } else {
        sprite->SetSize(IntVector2(256, 256));
        sprite->SetHotSpot(IntVector2(128, 128));
    }

    sprite->SetPriority(0);

    // Add as a child of the root UI element
    ui->GetRoot()->AddChild(sprite);

    SharedPtr<ObjectAnimation> animation(new ObjectAnimation(context_));
    SharedPtr<ValueAnimation> scale(new ValueAnimation(context_));
    // Use spline interpolation method
    scale->SetInterpolationMethod(IM_SPLINE);
    // Set spline tension
    scale->SetKeyFrame(0.0f, Vector2(1, 1));
    scale->SetKeyFrame(SPLASH_TIME / 1000 / 2, Vector2(1.5, 1.5));
    scale->SetKeyFrame(SPLASH_TIME / 1000, Vector2(1, 1));
    scale->SetKeyFrame(SPLASH_TIME / 1000 * 2, Vector2(1, 1));
    animation->AddAttributeAnimation("Scale", scale);

    SharedPtr<ValueAnimation> rotation(new ValueAnimation(context_));
    // Use spline interpolation method
    rotation->SetInterpolationMethod(IM_SPLINE);
    rotation->SetSplineTension(0.0f);
    // Set spline tension
    rotation->SetKeyFrame(0.0f, 0.0f);
    rotation->SetKeyFrame(1.0, 0.0f);
    rotation->SetKeyFrame(2.0, 360 * 1.0f);
    rotation->SetKeyFrame(SPLASH_TIME / 1000, 360 * 1.0f);
    rotation->SetKeyFrame(SPLASH_TIME / 1000 * 2, 360 * 1.0f);
    animation->AddAttributeAnimation("Rotation", rotation);

    sprite->SetObjectAnimation(animation);
}

void Splash::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Splash, HandleUpdate));
}

void Splash::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();
    if (input->IsMouseVisible()) {
        input->SetMouseVisible(false);
    }
    if (timer_.GetMSec(false) > SPLASH_TIME) {
        HandleEndSplash();
    }
}

void Splash::HandleEndSplash()
{
    UnsubscribeFromEvent(E_UPDATE);
    VariantMap& data = GetEventDataMap();
    logoIndex_++;
    if (logoIndex_ >= logos_.Size()) {
        data["Name"] = "MainMenu";
    } else {
        // We still have logos to show, inform next Splash screen to use the next logo from the `logos_` vector
        data["Name"] = "Splash";
        data["LogoIndex"] = logoIndex_;
    }
    SendEvent(E_SET_LEVEL, data);
}
