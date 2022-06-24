#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Resource/ResourceCache.h>
#include "PlayerState.h"
#include "PlayerEvents.h"
#include "../Globals/GUIDefines.h"
#include "../Globals/ViewLayers.h"

PlayerState::PlayerState(Context* context) :
        Component(context)
{
}

PlayerState::~PlayerState()
{
    VariantMap players = GetGlobalVar("Players").GetVariantMap();
    players.erase(ea::to_string(GetPlayerID()));
    SetGlobalVar("Players", players);
}

void PlayerState::RegisterObject(Context* context)
{
    context->RegisterFactory<PlayerState>();
    URHO3D_ACCESSOR_ATTRIBUTE("Score", GetScore, SetScore, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Player ID", GetPlayerID, SetPlayerID, int, -1, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Name", GetPlayerName, SetPlayerName, ea::string, "", AM_DEFAULT);
}

void PlayerState::OnNodeSet(Node* node)
{
    SubscribeToEvent(node, PlayerEvents::E_PLAYER_SCORE_ADD, URHO3D_HANDLER(PlayerState, HandlePlayerScoreAdd));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayerState, HandlePostUpdate));

    if (node) {
        auto *cache = GetSubsystem<ResourceCache>();
        label_ = node->GetParent()->CreateChild("Label", LOCAL);

        auto text3D = label_->CreateComponent<Text3D>();
        text3D->SetFont(cache->GetResource<Font>(APPLICATION_FONT), 30);
        text3D->SetColor(Color::GRAY);
        text3D->SetViewMask(VIEW_MASK_GUI);
        text3D->SetAlignment(HA_CENTER, VA_BOTTOM);
        text3D->SetFaceCameraMode(FaceCameraMode::FC_LOOKAT_Y);
//    text3D->SetViewMask(~(1 << _controllerId));

//    if (!SHOW_LABELS) {
//        label_->SetEnabled(false);
//    }
    }
}

int PlayerState::GetScore() const
{
    return score_;
}

void PlayerState::SetScore(int value)
{
    score_ = value;
    if (score_ < 0) {
        score_ = 0;
    }
    OnScoreChanged();
}

void PlayerState::AddScore(int value)
{
    score_ += value;
    if (score_ < 0) {
        score_ = 0;
    }

    VariantMap notificationData;
    if (value < 0) {
        notificationData["Status"] = "Error";
        notificationData["Message"] = name_ + " lost " + ea::string(-value) + " points";
    } else {
        notificationData["Message"] = name_ + " got " + ea::string(value) + " points";
    }
    SendEvent("ShowNotification", notificationData);

    OnScoreChanged();
}

void PlayerState::HandlePlayerScoreAdd(StringHash eventType, VariantMap& eventData)
{
    using namespace PlayerEvents::PlayerScoreAdd;
    int amount = eventData[P_SCORE].GetInt();
    URHO3D_LOGINFOF("Adding score to player %d", amount);
    AddScore(amount);
}

void PlayerState::OnScoreChanged()
{
    if (playerId_ >= 0) {
        VariantMap players = GetGlobalVar("Players").GetVariantMap();
        VariantMap playerData = players[ea::string(GetPlayerID())].GetVariantMap();
        playerData["Score"] = score_;
        playerData["ID"] = GetPlayerID();
        playerData["Name"] = GetPlayerName();
        players[ea::string(GetPlayerID())] = playerData;
        SetGlobalVar("Players", players);
        SendEvent(PlayerEvents::E_PLAYER_SCORES_UPDATED);
    }
    MarkNetworkUpdate();
}

void PlayerState::SetPlayerID(int id)
{
    playerId_ = id;
}

int PlayerState::GetPlayerID() const
{
    return playerId_;
}

void PlayerState::SetPlayerName(const ea::string& name)
{
    name_ = name;
    OnScoreChanged();
    MarkNetworkUpdate();

    if (label_) {
        label_->GetComponent<Text3D>()->SetText(name);
    }
}

const ea::string& PlayerState::GetPlayerName() const
{
    return name_;
}

void PlayerState::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (label_) {
        label_->SetPosition(node_->GetPosition() + Vector3::UP * 0.2);
    }
}

void PlayerState::HideLabel()
{
    if (label_) {
        label_->SetEnabled(false);
    }
}
