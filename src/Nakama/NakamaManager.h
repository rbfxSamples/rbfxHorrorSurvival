#pragma once
#ifdef NAKAMA_SUPPORT
    #include "PacketHandler.h"
    #include <Urho3D/Audio/Sound.h>
    #include <Urho3D/Audio/SoundSource3D.h>
    #include <Urho3D/Core/Object.h>
    #include <Urho3D/Core/Timer.h>
    #include <Urho3D/IO/Log.h>
    #include <Urho3D/Resource/ResourceCache.h>
    #include <Urho3D/Scene/Node.h>
    #include <Urho3D/UI/UIEvents.h>
    #include <nakama-cpp/Nakama.h>

using namespace Urho3D;
using namespace Nakama;

class NakamaManager : public Object
{
    URHO3D_OBJECT(NakamaManager, Object);

public:
    NakamaManager(Context* context);
    virtual ~NakamaManager();

    void Init();
    static void RegisterObject(Context* context);
    void LogIn(const ea::string& email, const ea::string& password);
    void Register(const ea::string& email, const ea::string& password, const ea::string& username);
    void CreateMatch();
    void JoinMatch(const ea::string& matchID);
    void LeaveMatch();
    void SendChatMessage(const ea::string& message);
    void SendData(int msgID, const VectorBuffer& data);

private:
    void SetupRTCClient();
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void ChannelMessageReceived(const NChannelMessage& message);
    void MatchDataReceived(const NMatchData& data);
    void MatchmakerMatched(NMatchmakerMatchedPtr matched);
    void MatchPresence(const NMatchPresenceEvent& event);
    void ChannelJoined(NChannelPtr channel);
    NClientPtr client_;
    NSessionPtr session_;
    NRtClientPtr rtClient_;
    NChannelPtr globalChatChannel_;
    Timer updateTimer_;
    VariantMap storage_;
    NRtDefaultClientListener rtclistener_;
    ea::string matchId_;
    PacketHandler packetHandler_;
};
#endif
