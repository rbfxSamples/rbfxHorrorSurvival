#include "BehaviourTree.h"
#include "../Console/ConsoleHandlerEvents.h"
#include "../Input/ControlDefines.h"
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;
using namespace ConsoleHandlerEvents;

BehaviourTree::BehaviourTree(Context* context)
    : LogicComponent(context)
{
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

BehaviourTree::~BehaviourTree() {}

void BehaviourTree::RegisterFactory(Context* context) { context->RegisterFactory<BehaviourTree>(); }

void BehaviourTree::Init(const ea::string& config)
{
    LoadConfig(config);
    SubscribeToEvents();
}

void BehaviourTree::LoadConfig(const ea::string& config)
{
    //    URHO3D_LOGINFOF("Loading behaviour tree: %s", config.c_str());
    auto json = GetSubsystem<ResourceCache>()->GetResource<JSONFile>(config);
    JSONValue& content = json->GetRoot();

    if (content.IsObject())
    {
        for (auto it = content.Begin(); it != content.End(); ++it)
        {
            // URHO3D_LOGWARNINGF("JSON: %s", (*it).first.c_str());
            auto field = (*it).first;
            auto value = (*it).second;

            if (field == "type")
            {
                if (value.IsString())
                {
                    if (value.GetString() == "Sequence")
                    {
                        rootNode_.nodeType = SEQUENCE;
                    }
                    else if (value.GetString() == "Selector")
                    {
                        rootNode_.nodeType = SELECTOR;
                    }
                }
            }
        }
    }
}

const Controls& BehaviourTree::GetControls() { return controls_; }

void BehaviourTree::FixedUpdate(float timeStep) { controls_.yaw_ += timeStep * 50; }

void BehaviourTree::SubscribeToEvents()
{
    controls_.Set(CTRL_FORWARD, true);
    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "behaviour_debug", ConsoleCommandAdd::P_EVENT,
        "#behaviour_debug", ConsoleCommandAdd::P_DESCRIPTION, "Change behaviour tree state");
    SubscribeToEvent(
        "#behaviour_debug", [&](StringHash eventType, VariantMap& eventData) { controls_.Set(CTRL_FORWARD, true); });
}
