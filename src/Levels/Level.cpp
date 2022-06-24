#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/SoundListener.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/UI.h>

#if !defined(__EMSCRIPTEN__)
    #include <Urho3D/Network/Network.h>
#endif

#include "../Audio/AudioEvents.h"
#include "../Audio/AudioManager.h"
#include "../Audio/AudioManagerDefs.h"
#include "../Console/ConsoleHandlerEvents.h"
#include "../CustomEvents.h"
#include "../Generator/Generator.h"
#include "../Globals/CollisionLayers.h"
#include "../Globals/ViewLayers.h"
#include "../Input/ControlDefines.h"
#include "../Input/ControllerEvents.h"
#include "../Input/ControllerInput.h"
#include "../LevelManagerEvents.h"
#include "../Messages/Achievements.h"
#include "../Network/NetworkEvents.h"
#include "../Player/PlayerEvents.h"
#include "../SceneManager.h"
#include "../UI/WindowEvents.h"
#include "../UI/WindowManager.h"
#include "Level.h"
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>

#ifdef VOXEL_SUPPORT
    #include "Voxel/ChunkGenerator.h"
    #include "Voxel/LightManager.h"
    #include "Voxel/TreeGenerator.h"
    #include "Voxel/VoxelEvents.h"
    #include "Voxel/VoxelWorld.h"
using namespace VoxelEvents;
#endif

using namespace Levels;
using namespace ConsoleHandlerEvents;
using namespace LevelManagerEvents;
using namespace ControllerEvents;
using namespace WindowEvents;
using namespace AudioEvents;
using namespace CustomEvents;
using namespace NetworkEvents;

static int REMOTE_PLAYER_ID = 1000;

Level::Level(Context* context)
    : BaseLevel(context)
    , drawDebug_(false)
{
}

Level::~Level()
{
    StopAllAudio();
#ifdef VOXEL_SUPPORT
    if (GetSubsystem<VoxelWorld>())
    {
        context_->RemoveSubsystem<VoxelWorld>();
        context_->RemoveSubsystem<ChunkGenerator>();
        context_->RemoveSubsystem<LightManager>();
        context_->RemoveSubsystem<TreeGenerator>();
    }
#endif
}

void Level::RegisterObject(Context* context)
{
    context->RegisterFactory<Level>();
    Player::RegisterObject(context);

#ifdef VOXEL_SUPPORT
    VoxelWorld::RegisterObject(context);
    Chunk::RegisterObject(context);
    ChunkGenerator::RegisterObject(context);
    LightManager::RegisterObject(context);
    TreeGenerator::RegisterObject(context);
#endif
}

void Level::Init()
{
    if (!scene_)
    {
        auto localization = GetSubsystem<Localization>();
        // There is no scene, get back to the main menu
        VariantMap& eventData = GetEventDataMap();
        eventData["Name"] = "MainMenu";
        eventData["Message"] = localization->Get("NO_SCENE");
        SendEvent(E_SET_LEVEL, eventData);

        return;
    }

    auto listener = scene_->CreateComponent<SoundListener>();
    GetSubsystem<Audio>()->SetListener(listener);
    // Enable achievement showing for this level
    GetSubsystem<Achievements>()->SetShowAchievements(true);

    StopAllAudio();
    StartAudio();

    auto* controllerInput = GetSubsystem<ControllerInput>();
    ea::vector<int> controlIndexes = controllerInput->GetControlIndexes();
    InitViewports(controlIndexes);

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

#ifdef VOXEL_SUPPORT
    if (data_.contains("Map") && data_["Map"].GetString() == "Scenes/Voxel.xml")
    {
        CreateVoxelWorld();
    }
#endif

    if (data_.contains("Map") && data_["Map"].GetString() == "Scenes/Terrain.xml")
    {
        auto cache = GetSubsystem<ResourceCache>();
        Node* terrainNode = scene_->CreateChild("Terrain");
        terrainNode->SetPosition(Vector3(0.0f, -0.0f, 0.0f));
        terrain_ = terrainNode->CreateComponent<Terrain>();
        terrain_->SetPatchSize(64);
        terrain_->SetSpacing(Vector3(1.0f, 0.1f, 1.0f));
        terrain_->SetSmoothing(true);
        terrain_->SetHeightMap(GetSubsystem<Generator>()->GetImage());
        terrain_->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
        terrain_->SetOccluder(true);
        terrain_->SetCastShadows(true);

        auto* body = terrainNode->CreateComponent<RigidBody>();
        body->SetCollisionLayerAndMask(COLLISION_MASK_GROUND, COLLISION_MASK_PLAYER | COLLISION_MASK_OBSTACLES);
        auto* shape = terrainNode->CreateComponent<CollisionShape>();
        shape->SetTerrain();
    }

    // Subscribe to global events for camera movement
    SubscribeToEvents();

    auto input = GetSubsystem<Input>();
    if (input->IsMouseVisible())
    {
        input->SetMouseVisible(false);
    }

    if (!GetSubsystem<Engine>()->IsHeadless())
    {
        for (auto it = controlIndexes.begin(); it != controlIndexes.end(); ++it)
        {
            using namespace RemoteClientId;
            if (data_.contains(P_NODE_ID) && data_.contains(P_PLAYER_ID))
            {
                // We are the client, we have to lookup the node on the received scene
                players_[(*it)] = CreatePlayer((*it), true, "", data_[P_NODE_ID].GetInt());
            }
            else
            {
                players_[(*it)] = CreatePlayer((*it), true, "Player " + ea::to_string((*it)));
            }
        }
    }

#if !defined(__EMSCRIPTEN__)
    if (!GetSubsystem<Network>()->GetServerConnection())
    {
        for (int i = 0; i < 0; i++)
        {
            players_[100 + i] = CreatePlayer(100 + i, false, "Bot " + ea::to_string(100 + i));
            URHO3D_LOGINFO("Bot created");
        }
    }
#endif

#ifdef __ANDROID__
    GetSubsystem<ControllerInput>()->ShowOnScreenJoystick();
#endif
}

#ifdef VOXEL_SUPPORT
void Level::CreateVoxelWorld()
{
    if (!GetSubsystem<VoxelWorld>())
    {
        context_->RegisterSubsystem(new VoxelWorld(context_));
    }
    if (!GetSubsystem<ChunkGenerator>())
    {
        context_->RegisterSubsystem(new ChunkGenerator(context_));
        GetSubsystem<ChunkGenerator>()->SetSeed(1);
    }
    if (!GetSubsystem<LightManager>())
    {
        context_->RegisterSubsystem(new LightManager(context_));
    }
    if (!GetSubsystem<TreeGenerator>())
    {
        context_->RegisterSubsystem(new TreeGenerator(context_));
    }
    GetSubsystem<VoxelWorld>()->Init();
}
#endif

SharedPtr<Player> Level::CreatePlayer(int controllerId, bool controllable, const ea::string& name, int nodeID)
{
    SharedPtr<Player> newPlayer(new Player(context_));

    auto mapInfo = GetSubsystem<SceneManager>()->GetCurrentMapInfo();
    if (mapInfo)
    {
        if (!mapInfo->startNode.empty())
        {
            Node* startNode = scene_->GetChild(mapInfo->startNode, true);
            if (startNode)
            {
                newPlayer->SetSpawnPoint(startNode->GetWorldPosition());
                URHO3D_LOGINFOF("Start point node");
            }
            else
            {
                newPlayer->SetSpawnPoint(mapInfo->startPoint);
            }
        }
        else
        {
            newPlayer->SetSpawnPoint(mapInfo->startPoint);
        }
    }

    if (nodeID > 0)
    {
        newPlayer->FindNode(scene_, nodeID);
    }
    else
    {
        newPlayer->CreateNode(scene_, controllerId, terrain_, 0);
    }
    newPlayer->SetControllable(controllable);
    newPlayer->SetControllerId(controllerId);
    if (!name.empty())
    {
        newPlayer->SetName(name);
    }

#ifdef VOXEL_SUPPORT
    if (GetSubsystem<ChunkGenerator>())
    {
        Vector3 spawnPoint = newPlayer->GetSpawnPoint();
        spawnPoint.y_ = GetSubsystem<ChunkGenerator>()->GetTerrainHeight(spawnPoint);
        spawnPoint.y_ += 5;
        newPlayer->SetSpawnPoint(spawnPoint);
        newPlayer->ResetPosition();
    }

    if (GetSubsystem<VoxelWorld>())
    {
        GetSubsystem<VoxelWorld>()->AddObserver(newPlayer->GetNode());
    }
#endif

    return newPlayer;
}

void Level::StartAudio()
{
    using namespace AudioDefs;
    using namespace PlaySound;
    VariantMap& data = GetEventDataMap();
    data[P_INDEX] = AMBIENT_SOUNDS::LEVEL;
    data[P_TYPE] = SOUND_AMBIENT;
    SendEvent(E_PLAY_SOUND, data);
}

void Level::StopAllAudio() { SendEvent(E_STOP_ALL_SOUNDS); }

void Level::CreateScene()
{
    if (!scene_->HasComponent<PhysicsWorld>())
    {
        scene_->CreateComponent<PhysicsWorld>(REPLICATED);
    }
}

void Level::CreateUI()
{
    auto* localization = GetSubsystem<Localization>();
    auto ui = GetSubsystem<UI>();

    Text* text = ui->GetRoot()->CreateChild<Text>();
    text->SetHorizontalAlignment(HA_CENTER);
    text->SetVerticalAlignment(VA_TOP);
    text->SetPosition(0, 50);
    text->SetStyleAuto();
    text->SetText(localization->Get("TUTORIAL"));
    text->SetTextEffect(TextEffect::TE_STROKE);
    text->SetFontSize(16);
    text->SetColor(Color(0.8f, 0.8f, 0.2f));
    text->SetBringToBack(true);
}

void Level::SubscribeToEvents()
{
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(Level, HandlePhysicsPrestep));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(Level, HandlePostUpdate));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(Level, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(Level, HandleKeyUp));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Level, HandlePostRenderUpdate));
    SubscribeToEvent(E_MAPPED_CONTROL_PRESSED, URHO3D_HANDLER(Level, HandleMappedControlPressed));

    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(Level, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(Level, HandleClientDisconnected));
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(Level, HandleServerConnected));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(Level, HandleServerDisconnected));

    SubscribeToEvent(E_CONTROLLER_ADDED, URHO3D_HANDLER(Level, HandleControllerConnected));
    SubscribeToEvent(E_CONTROLLER_REMOVED, URHO3D_HANDLER(Level, HandleControllerDisconnected));
    SubscribeToEvent(E_CONTROLLER_REMOVED, URHO3D_HANDLER(Level, HandleControllerDisconnected));

    SubscribeToEvent(E_VIDEO_SETTINGS_CHANGED, URHO3D_HANDLER(Level, HandleVideoSettingsChanged));

    SubscribeToEvent(PlayerEvents::E_SET_PLAYER_CAMERA_TARGET, URHO3D_HANDLER(Level, HandlePlayerTargetChanged));

    RegisterConsoleCommands();

    SubscribeToEvent(E_LEVEL_BEFORE_DESTROY, URHO3D_HANDLER(Level, HandleBeforeLevelDestroy));
    SubscribeToEvent("SettingsButtonPressed", [&](StringHash eventType, VariantMap& eventData) { ShowPauseMenu(); });
}

void Level::RegisterConsoleCommands()
{
    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "debug_geometry", ConsoleCommandAdd::P_EVENT,
        "#debug_geometry", ConsoleCommandAdd::P_DESCRIPTION, "Toggle debugging geometry",
        ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#debug_geometry",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() > 1)
            {
                URHO3D_LOGERROR("This command doesn't take any arguments!");
                return;
            }
            drawDebug_ = !drawDebug_;
        });

#ifdef VOXEL_SUPPORT
    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "seed", ConsoleCommandAdd::P_EVENT, "#seed",
        ConsoleCommandAdd::P_DESCRIPTION, "Change world seed", ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#seed",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() != 2)
            {
                URHO3D_LOGERROR("Seed parameter is required!");
                return;
            }
            int seed = ToInt(params[1]);
            if (!GetSubsystem<ChunkGenerator>())
            {
                GetSubsystem<ChunkGenerator>()->SetSeed(seed);
            }
        });
#endif
}

void Level::HandleBeforeLevelDestroy(StringHash eventType, VariantMap& eventData)
{
    remotePlayers_.clear();
#if !defined(__EMSCRIPTEN__)
    UnsubscribeFromEvent(E_SERVERDISCONNECTED);
    if (GetSubsystem<Network>() && GetSubsystem<Network>()->IsServerRunning())
    {
        GetSubsystem<Network>()->StopServer();
    }
    if (GetSubsystem<Network>() && GetSubsystem<Network>()->GetServerConnection())
    {
        GetSubsystem<Network>()->Disconnect();
    }
#endif
}

void Level::HandleControllerConnected(StringHash eventType, VariantMap& eventData)
{
#if !defined(__EMSCRIPTEN__)
    // TODO: allow to play splitscreen when connected to server
    if (GetSubsystem<Network>()->GetServerConnection())
    {
        URHO3D_LOGWARNING("Local splitscreen multiplayer is not yet supported in the network mode");
        return;
    }
#endif

    using namespace ControllerAdded;
    int controllerIndex = eventData[P_INDEX].GetInt();

    auto* controllerInput = GetSubsystem<ControllerInput>();
    ea::vector<int> controlIndexes = controllerInput->GetControlIndexes();
    InitViewports(controlIndexes);

    VariantMap& data = GetEventDataMap();
    data["Message"] = "New controller connected!";
    SendEvent("ShowNotification", data);

    if (!players_.contains(controllerIndex))
    {
        players_[controllerIndex] = CreatePlayer(controllerIndex, true, "Player " + ea::to_string(controllerIndex + 1));
    }
}

void Level::HandleControllerDisconnected(StringHash eventType, VariantMap& eventData)
{
#if !defined(__EMSCRIPTEN__)
    if (GetSubsystem<Network>()->GetServerConnection())
    {
        return;
    }
#endif

    using namespace ControllerRemoved;
    int controllerIndex = eventData[P_INDEX].GetInt();

    if (controllerIndex > 0)
    {
        players_.erase(controllerIndex);
    }
    auto* controllerInput = GetSubsystem<ControllerInput>();
    ea::vector<int> controlIndexes = controllerInput->GetControlIndexes();
    InitViewports(controlIndexes);

    VariantMap& data = GetEventDataMap();
    data["Message"] = "Controller disconnected!";
    SendEvent("ShowNotification", data);
}

void Level::UnsubscribeToEvents()
{
    UnsubscribeFromEvent(E_PHYSICSPRESTEP);
    UnsubscribeFromEvent(E_POSTUPDATE);
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_KEYUP);
}

void Level::HandlePhysicsPrestep(StringHash eventType, VariantMap& eventData)
{
    if (!scene_->IsUpdateEnabled())
    {
        return;
    }
}

void Level::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!drawDebug_)
    {
        return;
    }
    scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
    scene_->GetComponent<PhysicsWorld>()->SetDebugRenderer(scene_->GetComponent<DebugRenderer>());
    //    scene_->GetComponent<Octree>()->DrawDebugGeometry(scene_->GetComponent<DebugRenderer>(), true);
}

void Level::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{

    if (!scene_->IsUpdateEnabled())
    {
        return;
    }

    using namespace PostUpdate;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    auto* controllerInput = GetSubsystem<ControllerInput>();
    for (auto it = players_.begin(); it != players_.end(); ++it)
    {

        int playerId = (*it).first;
        Controls controls = controllerInput->GetControls((*it).first);
        if (cameras_[playerId])
        {
            Quaternion rotation(controls.pitch_, controls.yaw_, 0.0f);
            cameras_[playerId]->SetRotation(rotation);

            Node* target = (*it).second->GetCameraTarget();
            if (target)
            {
                // Move camera some distance away from the ball
                cameras_[playerId]->SetPosition(target->GetPosition()
                    + cameras_[playerId]->GetRotation() * Vector3::BACK * (*it).second->GetCameraDistance());
            }
        }
    }
}

void Level::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    int key = eventData["Key"].GetInt();

    if (key == KEY_TAB && !showScoreboard_)
    {
        VariantMap& data = GetEventDataMap();
        data["Name"] = "ScoreboardWindow";
        SendEvent(E_OPEN_WINDOW, data);
        showScoreboard_ = true;
    }

    if (key == KEY_ESCAPE)
    {
        ShowPauseMenu();
    }
}

void Level::ShowPauseMenu()
{
    VariantMap& data = GetEventDataMap();
    data["Name"] = "PauseWindow";
    SendEvent(E_OPEN_WINDOW, data);

#if !defined(__EMSCRIPTEN__)
    if (!GetSubsystem<Network>()->IsServerRunning() && !GetSubsystem<Network>()->GetServerConnection())
    {
        UnsubscribeToEvents();
        SubscribeToEvent(E_WINDOW_CLOSED, URHO3D_HANDLER(Level, HandleWindowClosed));
        Pause();
    }
#endif
}

void Level::PauseMenuHidden()
{
    UnsubscribeFromEvent(E_WINDOW_CLOSED);
    SubscribeToEvents();

    Input* input = GetSubsystem<Input>();
    if (input->IsMouseVisible())
    {
        input->SetMouseVisible(false);
    }
    Run();
}

void Level::HandleKeyUp(StringHash eventType, VariantMap& eventData)
{
    int key = eventData["Key"].GetInt();

    if (key == KEY_TAB && showScoreboard_)
    {
        VariantMap& data = GetEventDataMap();
        data["Name"] = "ScoreboardWindow";
        SendEvent(E_CLOSE_WINDOW, data);
        showScoreboard_ = false;
    }
}

void Level::HandleWindowClosed(StringHash eventType, VariantMap& eventData)
{
    ea::string name = eventData["Name"].GetString();
    if (name == "PauseWindow")
    {
        PauseMenuHidden();
    }
    if (!GetSubsystem<WindowManager>()->IsAnyWindowOpened())
    {
        auto input = GetSubsystem<Input>();
        input->SetMouseVisible(false);
    }
}

void Level::HandleVideoSettingsChanged(StringHash eventType, VariantMap& eventData)
{
    auto* controllerInput = GetSubsystem<ControllerInput>();
    ea::vector<int> controlIndexes = controllerInput->GetControlIndexes();
    InitViewports(controlIndexes);
}

void Level::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client connects, assign to scene to begin scene replication
    auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(scene_);

    remotePlayers_[newConnection] = CreatePlayer(REMOTE_PLAYER_ID, true, "Remote " + ea::to_string(REMOTE_PLAYER_ID));
    remotePlayers_[newConnection]->SetClientConnection(newConnection);
    REMOTE_PLAYER_ID++;

    using namespace RemoteClientId;
    VariantMap data;
    data[P_NODE_ID] = remotePlayers_[newConnection]->GetNode()->GetID();
    data[P_PLAYER_ID] = remotePlayers_[newConnection]->GetControllerId();
    URHO3D_LOGINFOF("Sending out remote client id %d", remotePlayers_[newConnection]->GetNode()->GetID());
    newConnection->SendRemoteEvent(E_REMOTE_CLIENT_ID, true, data);
}

void Level::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("Client disconnected");
    using namespace ClientDisconnected;

    // When a client connects, assign to scene to begin scene replication
    auto* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    remotePlayers_.erase(connection);
}

void Level::HandleServerConnected(StringHash eventType, VariantMap& eventData) {}

void Level::HandleServerDisconnected(StringHash eventType, VariantMap& eventData)
{
    players_.clear();
    auto localization = GetSubsystem<Localization>();
    VariantMap data;
    data["Name"] = "MainMenu";
    data["Message"] = localization->Get("DISCONNECTED_FROM_SERVER");
    data["Type"] = "error";
    SendEvent(E_SET_LEVEL, data);
}

void Level::HandlePlayerTargetChanged(StringHash eventType, VariantMap& eventData)
{
    using namespace PlayerEvents::SetPlayerCameraTarget;
    int playerId = eventData[P_ID].GetInt();
    Node* targetNode = nullptr;
    float cameraDistance = 0.0f;
    if (eventData.contains(P_NODE))
    {
        targetNode = static_cast<Node*>(eventData[P_NODE].GetPtr());
    }
    if (eventData.contains(P_DISTANCE))
    {
        cameraDistance = eventData[P_DISTANCE].GetFloat();
    }
    if (players_.contains(playerId))
    {
        players_[playerId]->SetCameraTarget(targetNode);
        players_[playerId]->SetCameraDistance(cameraDistance);
    }
}

bool Level::RaycastFromCamera(
    Camera* camera, float maxDistance, Vector3& hitPos, Vector3& hitNormal, Drawable*& hitDrawable)
{
    hitDrawable = nullptr;

    UI* ui = GetSubsystem<UI>();
    Input* input = GetSubsystem<Input>();
    IntVector2 pos = ui->GetUICursorPosition();
    // Check the cursor is visible and there is no UI element in front of the cursor
    if (input->IsMouseVisible() || ui->GetElementAt(pos, true))
        return false;

    Graphics* graphics = GetSubsystem<Graphics>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());
    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    ea::vector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY, VIEW_MASK_CHUNK);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.size())
    {
        RayQueryResult& result = results[0];
        hitPos = result.position_;
        hitNormal = result.normal_;
        hitDrawable = result.drawable_;
        return true;
    }

    return false;
}

void Level::HandleMappedControlPressed(StringHash eventType, VariantMap& eventData)
{
    using namespace MappedControlPressed;
    int action = eventData[P_ACTION].GetInt();
    if (action == CTRL_ACTION)
    {
        int controllerId = eventData[P_CONTROLLER].GetInt();
        if (cameras_.contains(controllerId))
        {
            Camera* camera = cameras_[controllerId]->GetComponent<Camera>();
            Vector3 hitPosition;
            Vector3 hitNormal;
            Drawable* hitDrawable;
            bool hit = RaycastFromCamera(camera, 100.0f, hitPosition, hitNormal, hitDrawable);
            if (hit)
            {
#ifdef VOXEL_SUPPORT
                using namespace ChunkHit;
                VariantMap& data = GetEventDataMap();
                data[P_POSITION] = hitPosition - hitNormal * 0.5f;
                data[P_DIRECTION] = hitNormal * 0.5f;
                data[P_ORIGIN] = hitPosition + hitNormal * 0.5f;
                ;
                data[P_CONTROLLER_ID] = eventData[P_CONTROLLER];
                data[P_ACTION_ID] = action;
                hitDrawable->GetNode()->GetParent()->SendEvent(E_CHUNK_HIT, data);
#endif
            }
        }
    }
    else if (action == CTRL_SECONDARY || action == CTRL_DETECT)
    {
        int controllerId = eventData[P_CONTROLLER].GetInt();
        if (cameras_.contains(controllerId))
        {
            Camera* camera = cameras_[controllerId]->GetComponent<Camera>();
            Vector3 hitPosition;
            Vector3 hitNormal;
            Drawable* hitDrawable;
            bool hit = RaycastFromCamera(camera, 100.0f, hitPosition, hitNormal, hitDrawable);
            if (hit)
            {
                //URHO3D_LOGINFO("Hit target " + hitDrawable->GetNode()->GetName() + " Normal: " +
                //hitNormal.ToString());
                Vector3 playerPosition = players_[controllerId]->GetNode()->GetWorldPosition();
                Vector3 blockPosition = hitPosition + hitNormal * 0.5f;
                //URHO3D_LOGINFO("Player position " + playerPosition.ToString() + " block position " +
                //blockPosition.ToString());
                if (Floor(blockPosition.x_) != Floor(playerPosition.x_)
                    || Floor(blockPosition.y_) != Floor(playerPosition.y_)
                    || Floor(blockPosition.z_) != Floor(playerPosition.z_))
                {
#ifdef VOXEL_SUPPORT
                    using namespace ChunkAdd;
                    VariantMap& data = GetEventDataMap();
                    data[P_POSITION] = blockPosition;
                    data[P_CONTROLLER_ID] = eventData[P_CONTROLLER];
                    data[P_ACTION_ID] = action;
                    data[P_ITEM_ID] = players_[controllerId]->GetSelectedItem();
                    hitDrawable->GetNode()->GetParent()->SendEvent(E_CHUNK_ADD, data);
#endif
                }
                else
                {
                    URHO3D_LOGINFO("You cannot place a block where you stand");
                }
            }
        }
    }
}
