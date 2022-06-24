#ifdef VOXEL_SUPPORT
    #include <Urho3D/Container/ea::vector.h>
    #include <Urho3D/Core/Context.h>
    #include <Urho3D/Core/CoreEvents.h>
    #include <Urho3D/Engine/DebugHud.h>
    #include <Urho3D/Graphics/Octree.h>
    #include <Urho3D/IO/FileSystem.h>
    #include <Urho3D/IO/Log.h>
    #include <Urho3D/Math/Ray.h>

    #if !defined(__EMSCRIPTEN__)
        #include <Urho3D/Network/Network.h>
        #include <Urho3D/Network/NetworkEvents.h>
    #endif

    #include "../../Console/ConsoleHandlerEvents.h"
    #include "../../SceneManager.h"
    #include "LightManager.h"
    #include "TreeGenerator.h"
    #include "VoxelEvents.h"
    #include "VoxelWorld.h"
    #include <Urho3D/Graphics/Material.h>
    #include <Urho3D/Resource/ResourceCache.h>

using namespace VoxelEvents;
using namespace ConsoleHandlerEvents;

bool CompareChunks(const Chunk* lhs, const Chunk* rhs) { return lhs->GetDistance() < rhs->GetDistance(); }

void UpdateChunkState(const WorkItem* item, unsigned threadIndex)
{
    Timer loadTime;
    VoxelWorld* world = reinterpret_cast<VoxelWorld*>(item->aux_);
    MutexLock lock(world->mutex_);
    if (world->GetSubsystem<LightManager>())
    {
        world->GetSubsystem<LightManager>()->ResetFailedCalculations();
        world->GetSubsystem<LightManager>()->Process();
    }
    if (world->GetSubsystem<TreeGenerator>())
    {
        world->GetSubsystem<TreeGenerator>()->Process();
    }
    if (world->GetSubsystem<DebugHud>())
    {
        world->GetSubsystem<DebugHud>()->SetAppStats("Chunks Loaded", world->chunks_.size());
    }

    int counter = 0;
    for (auto chIt = world->chunks_.begin(); chIt != world->chunks_.end(); ++chIt)
    {
        if ((*chIt).second && (*chIt).second->IsActive())
        {
            counter++;
        }
    }
    if (world->GetSubsystem<DebugHud>())
    {
        world->GetSubsystem<DebugHud>()->SetAppStats("Active chunks", counter);
    }

    int requestedFromServerCount = 0;
    int savePerFrame = 0;

    ea::vector<Chunk*> chunks;
    for (auto it = world->chunks_.begin(); it != world->chunks_.end(); ++it)
    {
        if ((*it).second)
        {
            chunks.push_back((*it).second.Get());
        }
    }

    Sort(chunks.begin(), chunks.end(), CompareChunks);
    for (auto it = chunks.begin(); it != chunks.end(); ++it)
    {

        if (world->reloadAllChunks_)
        {
            (*it)->MarkForGeometryCalculation();
        }
        // Initialize new chunks
        if (!(*it)->IsLoaded())
        {
    #if !defined(__EMSCRIPTEN__)
            if (!world->GetSubsystem<Network>()->GetServerConnection())
            {
                (*it)->Load();
                //                for (int i = 0; i < 6; i++) {
                //                    auto neighbor = (*it).second->GetNeighbor(static_cast<BlockSide>(i));
                //                    if (neighbor) {
                ////                        neighbor->CalculateLight();
                //                    }
                //                }
            }
            else if (!(*it)->IsRequestedFromServer())
            {
                (*it)->LoadFromServer();
                requestedFromServerCount++;
            }
    #else
            (*it)->Load();
    #endif
        }

        if (!(*it)->IsGeometryCalculated())
        {
            (*it)->CalculateGeometry();
            //            URHO3D_LOGINFO("CalculateGeometry " + (*it).second->GetPosition().ToString());
        }

        if ((*it)->ShouldSave() && savePerFrame < 1)
        {
            (*it)->Save();
            savePerFrame++;
        }
    }

    world->ProcessQueue();

    world->reloadAllChunks_ = false;
    //    URHO3D_LOGINFO("Chunks updated in " + ea::string(loadTime.GetMSec(false)) + "ms");
}

VoxelWorld::VoxelWorld(Context* context)
    : Object(context)
{
}

void VoxelWorld::Init()
{
    scene_ = GetSubsystem<SceneManager>()->GetActiveScene();

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(VoxelWorld, HandleUpdate));
    SubscribeToEvent(E_CHUNK_RECEIVED, URHO3D_HANDLER(VoxelWorld, HandleChunkReceived));
    SubscribeToEvent(E_WORKITEMCOMPLETED, URHO3D_HANDLER(VoxelWorld, HandleWorkItemFinished));

    #if !defined(__EMSCRIPTEN__)
    SubscribeToEvent(E_NETWORKMESSAGE, URHO3D_HANDLER(VoxelWorld, HandleNetworkMessage));
    #endif

    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "chunk_visible_distance", ConsoleCommandAdd::P_EVENT,
        "#chunk_visible_distance", ConsoleCommandAdd::P_DESCRIPTION, "How far away the generated chunks are visible",
        ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#chunk_visible_distance",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() != 2)
            {
                URHO3D_LOGERROR("radius parameter is required!");
                return;
            }
            int value = ToInt(params[1]);
            visibleDistance_ = value;
            URHO3D_LOGINFOF("Changing chunk visibility radius to %d", value);
        });

    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "world_reset", ConsoleCommandAdd::P_EVENT,
        "#world_reset", ConsoleCommandAdd::P_DESCRIPTION, "Remove saved chunks", ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#world_reset",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() != 1)
            {
                URHO3D_LOGERROR("This command doesn't have any arguments!");
                return;
            }
            if (GetSubsystem<FileSystem>()->DirExists("World"))
            {
                ea::vector<ea::string> files;
                GetSubsystem<FileSystem>()->ScanDir(files, "World", "", SCAN_FILES, false);
                for (auto it = files.begin(); it != files.end(); ++it)
                {
                    URHO3D_LOGINFO("Deleting file " + (*it));
                    GetSubsystem<FileSystem>()->Delete("World/" + (*it));
                }
            }
        });

    SendEvent(E_CONSOLE_COMMAND_ADD, ConsoleCommandAdd::P_NAME, "sunlight", ConsoleCommandAdd::P_EVENT, "#sunlight",
        ConsoleCommandAdd::P_DESCRIPTION, "Set sunlight level [0-15]", ConsoleCommandAdd::P_OVERWRITE, true);
    SubscribeToEvent("#sunlight",
        [&](StringHash eventType, VariantMap& eventData)
        {
            StringVector params = eventData["Parameters"].GetStringVector();
            if (params.size() != 2)
            {
                URHO3D_LOGERROR("Thi command requires exactly 1 argument!");
                return;
            }
            SetSunlight(ToFloat(params[1]));
        });
}

void VoxelWorld::RegisterObject(Context* context) { context->RegisterFactory<VoxelWorld>(); }

void VoxelWorld::AddObserver(SharedPtr<Node> observer)
{
    observers_.push_back(observer);
    URHO3D_LOGINFO("Adding observer to voxel world!");
}

void VoxelWorld::RemoveObserver(SharedPtr<Node> observer)
{
    auto it = observers_.find(observer);
    if (it != observers_.end())
    {
        observers_.erase(it);
        URHO3D_LOGINFO("Removing observer from voxel world!");
    }
}

void VoxelWorld::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    int loadedChunkCounter = 0;
    if (!removeBlocks_.empty())
    {
        auto chunk = GetChunkByPosition(removeBlocks_.front());
        if (chunk)
        {
            VariantMap& data = GetEventDataMap();
            data["Position"] = removeBlocks_.front();
            data["ControllerId"] = -1;
            if (chunk->GetNode())
            {
                chunk->GetNode()->SendEvent("ChunkHit", data);
                removeBlocks_.pop_front();
            }
        }
    }

    UpdateChunks();

    SetSunlight(Sin(GetSubsystem<Time>()->GetElapsedTime() * 10.0f) * 0.5f + 0.5f);
}

Chunk* VoxelWorld::CreateChunk(const Vector3& position)
{
    ea::string id = GetChunkIdentificator(position);
    chunks_[id] = new Chunk(context_);
    chunks_[id]->Init(scene_, position);
    return chunks_[id].Get();
}

Vector3 VoxelWorld::GetNodeToChunkPosition(Node* node)
{
    Vector3 position = node->GetWorldPosition();
    return GetWorldToChunkPosition(position);
}

bool VoxelWorld::IsChunkLoaded(const Vector3& position)
{
    ea::string id = GetChunkIdentificator(position);
    if (chunks_.contains(id) && chunks_[id])
    {
        return true;
    }
    return false;
}

void VoxelWorld::LoadChunk(const Vector3& position)
{
    //    Vector3 fixedChunkPosition = GetWorldToChunkPosition(position);
    //    if (!IsChunkLoaded(fixedChunkPosition)) {
    //        AddChunkToQueue(position);
    //    }
    //    ea::vector<Vector3> positions;

    // Terrain blocks only
    //    for (int x = -5; x < 5; x++) {
    //        for (int z = -5; z < 5; z++) {
    //            Vector3 terrain = Vector3(fixedChunkPosition + Vector3::FORWARD * SIZE_Z * z + Vector3::LEFT * SIZE_X
    //            * x); terrain.y_ = GetSubsystem<ChunkGenerator>()->GetTerrainHeight(terrain);
    //            positions.push_back(terrain);
    ////            positions.push_back(terrain + Vector3::UP * SIZE_Y);
    //            positions.push_back(terrain + Vector3::DOWN * SIZE_Y);
    //        }
    //    }
    //
    //    // Same
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::RIGHT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::FORWARD * SIZE_Z));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::BACK * SIZE_Z));
    //
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::BACK * SIZE_Z + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::BACK * SIZE_Z + Vector3::RIGHT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::FORWARD * SIZE_Z + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::FORWARD * SIZE_Z + Vector3::RIGHT * SIZE_X));
    //
    //    // Up
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::RIGHT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::FORWARD * SIZE_Z));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::BACK * SIZE_Z));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y));
    //
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::BACK * SIZE_Z + Vector3::LEFT
    //    * SIZE_X)); positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y + Vector3::BACK * SIZE_Z +
    //    Vector3::RIGHT * SIZE_X)); positions.push_back(Vector3(fixedChunkPosition + Vector3::UP * SIZE_Y +
    //    Vector3::FORWARD * SIZE_Z + Vector3::LEFT * SIZE_X)); positions.push_back(Vector3(fixedChunkPosition +
    //    Vector3::UP * SIZE_Y + Vector3::FORWARD * SIZE_Z + Vector3::RIGHT * SIZE_X));
    //
    //    // Down
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::RIGHT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::FORWARD * SIZE_Z));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::BACK * SIZE_Z));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y));
    //
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::BACK * SIZE_Z +
    //    Vector3::LEFT * SIZE_X)); positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y +
    //    Vector3::BACK * SIZE_Z + Vector3::RIGHT * SIZE_X)); positions.push_back(Vector3(fixedChunkPosition +
    //    Vector3::DOWN * SIZE_Y + Vector3::FORWARD * SIZE_Z + Vector3::LEFT * SIZE_X));
    //    positions.push_back(Vector3(fixedChunkPosition + Vector3::DOWN * SIZE_Y + Vector3::FORWARD * SIZE_Z +
    //    Vector3::RIGHT * SIZE_X));
    //
    //    for (auto it = positions.begin(); it != positions.end(); ++it) {
    //        if(!IsChunkLoaded((*it))) {
    //            CreateChunk((*it));
    //        }
    //    }
}

bool VoxelWorld::IsEqualPositions(Vector3 a, Vector3 b)
{
    return Floor(a.x_) == Floor(b.x_) && Floor(a.y_) == Floor(b.y_) && Floor(a.z_) == Floor(b.z_);
}

Chunk* VoxelWorld::GetChunkByPosition(const Vector3& position)
{
    Vector3 fixedPositon = GetWorldToChunkPosition(position);
    ea::string id = GetChunkIdentificator(fixedPositon);
    if (chunks_.find(id) != chunks_.end() && chunks_[id])
    {
        return chunks_[id].Get();
    }

    return nullptr;
}

Vector3 VoxelWorld::GetWorldToChunkPosition(const Vector3& position)
{
    return Vector3(Floor(position.x_ / SIZE_X) * SIZE_X, Floor(position.y_ / SIZE_Y) * SIZE_Y,
        Floor(position.z_ / SIZE_Z) * SIZE_Z);
}

IntVector3 VoxelWorld::GetWorldToChunkBlockPosition(const Vector3& position)
{
    Vector3 chunkPosition = GetWorldToChunkPosition(position);
    IntVector3 blockPosition;
    blockPosition.x_ = Floor(position.x_ - chunkPosition.x_);
    blockPosition.y_ = Floor(position.y_ - chunkPosition.y_);
    blockPosition.z_ = Floor(position.z_ - chunkPosition.z_);
    return blockPosition;
}

void VoxelWorld::RemoveBlockAtPosition(const Vector3& position) { removeBlocks_.push_back(position); }

void VoxelWorld::UpdateChunks()
{
    if (!updateWorkItem_)
    {
        if (!chunksToLoad_.empty())
        {
            for (auto it = chunks_.begin(); it != chunks_.end(); ++it)
            {
                if ((*it).second)
                {
                    (*it).second->MarkForDeletion(true);
                    (*it).second->SetDistance(-1);
                }
            }
        }

        for (auto it = chunksToLoad_.begin(); it != chunksToLoad_.end(); ++it)
        {
            Vector3 position = (*it).first;
            ea::string id = GetChunkIdentificator(position);
            auto chunkIterator = chunks_.find(id);
            if (chunkIterator != chunks_.end())
            {
                (*chunkIterator).second->MarkForDeletion(false);
                (*chunkIterator).second->SetDistance((*it).second);
            }
            else
            {
                auto chunk = CreateChunk(position);
                chunk->SetDistance((*it).second);
            }
        }

        chunksToLoad_.clear();

        MutexLock lock(mutex_);
        for (auto it = chunks_.begin(); it != chunks_.end(); ++it)
        {
            if ((*it).second)
            {
                if ((*it).second->IsMarkedForDeletion())
                {
                    int distance = (*it).second->GetDistance();
                    //                        URHO3D_LOGINFOF("Deleting chunk distance=%d ", distance);
                    it = chunks_.erase(it);
                }
                if (chunks_.end() == it)
                {
                    break;
                }
            }
        }

        WorkQueue* workQueue = GetSubsystem<WorkQueue>();
        updateWorkItem_ = workQueue->GetFreeItem();
        updateWorkItem_->priority_ = M_MAX_INT;
        updateWorkItem_->workFunction_ = UpdateChunkState;
        updateWorkItem_->aux_ = this;
        updateWorkItem_->sendEvent_ = true;
        updateWorkItem_->start_ = nullptr;
        updateWorkItem_->end_ = nullptr;
        workQueue->AddWorkItem(updateWorkItem_);
    }

    int renderedChunkCount = 0;
    int renderedChunkLimit = 1;
    for (auto it = chunks_.begin(); it != chunks_.end(); ++it)
    {
        if ((*it).second->ShouldRender())
        {
            bool rendered = (*it).second->Render();
            //            URHO3D_LOGINFO("Rendering chunk " + (*it).second->GetPosition().ToString());
            if (rendered)
            {
                renderedChunkCount++;
            }
            if (renderedChunkCount >= renderedChunkLimit)
            {
                break;
            }
        }
    }
}

ea::string VoxelWorld::GetChunkIdentificator(const Vector3& position)
{
    return ea::string((int)position.x_) + "_" + ea::string((int)position.y_) + "_" + ea::string((int)position.z_);
}

VoxelBlock* VoxelWorld::GetBlockAt(Vector3 position)
{
    Vector3 chunkPosition = GetWorldToChunkPosition(position);
    ea::string id = GetChunkIdentificator(chunkPosition);
    if (chunks_.contains(id) && chunks_[id])
    {
        Vector3 blockPosition = position - chunkPosition;
        return chunks_[id]->GetBlockAt(IntVector3(blockPosition.x_, blockPosition.y_, blockPosition.z_));
    }
    return nullptr;
}

bool VoxelWorld::IsChunkValid(Chunk* chunk)
{
    for (auto it = chunks_.begin(); it != chunks_.end(); ++it)
    {
        if ((*it).second.Get() == chunk)
        {
            return true;
        }
    }

    return false;
}

void VoxelWorld::HandleWorkItemFinished(StringHash eventType, VariantMap& eventData)
{
    using namespace WorkItemCompleted;
    WorkItem* workItem = reinterpret_cast<WorkItem*>(eventData[P_ITEM].GetPtr());
    if (workItem->aux_ != this)
    {
        return;
    }
    //    if (workItem->workFunction_ == GenerateVoxelData) {
    //        CreateNode();
    //        generateWorkItem_.Reset();
    //        MarkDirty();
    //    } else if (workItem->workFunction_ == GenerateVertices) {
    ////        GetSubsystem<VoxelWorld>()->AddChunkToRenderQueue(this);
    //        Render();
    //        generateGeometryWorkItem_.Reset();
    //    } else
    if (workItem->workFunction_ == UpdateChunkState)
    {
        updateWorkItem_.Reset();
    }
}

const ea::string VoxelWorld::GetBlockName(BlockType type)
{
    switch (type)
    {
    case BT_AIR: return "BT_AIR";
    case BT_STONE: return "BT_STONE";
    case BT_DIRT: return "BT_DIRT";
    case BT_SAND: return "BT_SAND";
    case BT_COAL: return "BT_COAL";
    case BT_TORCH: return "BT_TORCH";
    case BT_WOOD: return "BT_WOOD";
    case BT_TREE_LEAVES: return "BT_TREE_LEAVES";
    case BT_WATER: return "BT_WATER";
    default: return "BT_NONE";
    }
}

bool VoxelWorld::ProcessQueue()
{
    if (updateTimer_.GetMSec(false) < 100)
    {
        return false;
    }
    updateTimer_.Reset();

    bool haveChanges = false;

    for (auto it = observers_.begin(); it != observers_.end(); ++it)
    {
        Vector3 currentChunkPosition = GetWorldToChunkPosition((*it)->GetWorldPosition());
        if (!(*it)->GetVar("ChunkPosition").IsEmpty())
        {
            Vector3 lastChunkPosition = (*it)->GetVar("ChunkPosition").GetVector3();
            if (lastChunkPosition.x_ != currentChunkPosition.x_ || lastChunkPosition.y_ != currentChunkPosition.y_
                || lastChunkPosition.z_ != currentChunkPosition.z_)
            {
                haveChanges = true;
            }
        }
        else
        {
            haveChanges = true;
        }
        if (haveChanges)
        {
            (*it)->SetVar("ChunkPosition", currentChunkPosition);
            URHO3D_LOGINFOF("Player %s moved to chunk %dx%dx%d", (*it)->GetName().c_str(), (int)currentChunkPosition.x_,
                (int)currentChunkPosition.y_, (int)currentChunkPosition.z_);
        }
    }

    if (!haveChanges)
    {
        return false;
    }

    for (auto it = observers_.begin(); it != observers_.end(); ++it)
    {
        AddChunkToQueue(((*it)->GetWorldPosition()));
    }

    while (!chunkBfsQueue_.empty())
    {
        ChunkNode& node = chunkBfsQueue_.front();
        int distance = node.distance_ + 1;
        chunkBfsQueue_.pop();
        if (distance <= visibleDistance_)
        {
            AddChunkToQueue(node.position_ + Vector3::LEFT * SIZE_X, distance);
            AddChunkToQueue(node.position_ + Vector3::RIGHT * SIZE_X, distance);
            AddChunkToQueue(node.position_ + Vector3::FORWARD * SIZE_Z, distance);
            AddChunkToQueue(node.position_ + Vector3::BACK * SIZE_Z, distance);
            AddChunkToQueue(node.position_ + Vector3::UP * SIZE_Y, distance);
            AddChunkToQueue(node.position_ + Vector3::DOWN * SIZE_Y, distance);
        }
    }

    return true;
}

void VoxelWorld::AddChunkToQueue(Vector3 position, int distance)
{
    Vector3 fixedPosition = GetWorldToChunkPosition(position);
    ea::string id = GetChunkIdentificator(fixedPosition);
    ChunkNode node(position, distance);
    if (!chunksToLoad_.contains(fixedPosition))
    {
        chunksToLoad_[fixedPosition] = distance;
        chunkBfsQueue_.emplace(node);
    }
}

void VoxelWorld::HandleChunkReceived(StringHash eventType, VariantMap& eventData)
{
    using namespace ChunkReceived;
    Vector3 position = eventData[P_POSITION].GetVector3();
    URHO3D_LOGINFO("Chunk received: " + position.ToString());
    PODVector<unsigned char>* data = reinterpret_cast<PODVector<unsigned char>*>(eventData[P_DATA].GetPtr());
    ea::string id = GetChunkIdentificator(position);
    auto chunkIterator = chunks_.find(id);
    if (chunkIterator != chunks_.end())
    {
        int index = 0;
        for (int x = 0; x < SIZE_X; x++)
        {
            for (int y = 0; y < SIZE_Y; y++)
            {
                for (int z = 0; z < SIZE_Z; z++)
                {
                    int value = data->At(index);
                    BlockType type = static_cast<BlockType>(value);
                    (*chunkIterator).second->SetVoxel(x, y, z, type);
                }
            }
        }
        (*chunkIterator).second->CalculateLight();
        (*chunkIterator).second->MarkForGeometryCalculation();
    }
}

    #if !defined(__EMSCRIPTEN__)
void VoxelWorld::HandleNetworkMessage(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();

    using namespace NetworkMessage;

    int msgID = eventData[P_MESSAGEID].GetInt();
    if (msgID == NETWORK_REQUEST_CHUNK)
    {
        const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
        // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
        MemoryBuffer msg(data);
        Vector3 chunkPosition = msg.ReadVector3();

        Chunk* chunk = GetChunkByPosition(chunkPosition);
        // If we are the server, prepend the sender's IP address and port and echo to everyone
        // If we are a client, just display the message
        if (network->IsServerRunning())
        {
            if (chunk)
            {
                if (chunk->IsLoaded())
                {
                    auto* sender = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
                    //                URHO3D_LOGINFO("Client " + sender->ToString() + " requested chunk : " +
                    //                chunkPosition.ToString());

                    VectorBuffer sendMsg;
                    sendMsg.WriteVector3(chunk->GetPosition());
                    for (int x = 0; x < SIZE_X; x++)
                    {
                        for (int y = 0; y < SIZE_Y; y++)
                        {
                            for (int z = 0; z < SIZE_Z; z++)
                            {
                                sendMsg.WriteInt(static_cast<int>(chunk->GetBlockAt(IntVector3(x, y, z))->type));
                            }
                        }
                    }
                    // Broadcast as in-order and reliable
                    sender->SendMessage(NETWORK_SEND_CHUNK, true, true, sendMsg);
                }
                else
                {
                    //                    URHO3D_LOGINFO("Chunk not yet loaded, cannot send it to client " +
                    //                    chunkPosition.ToString());
                }
            }
            else
            {
                //                URHO3D_LOGINFO("Requested chunk doesn't exist " + chunkPosition.ToString());
            }
        }
    }
    else if (msgID == NETWORK_SEND_CHUNK)
    {
        if (!network->IsServerRunning())
        {
            const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
            // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
            MemoryBuffer msg(data);
            Vector3 chunkPosition = msg.ReadVector3();
            auto chunk = GetChunkByPosition(chunkPosition);
            if (chunk)
            {
                chunk->ProcessServerResponse(msg);
            }
            else
            {
                URHO3D_LOGINFO("Requested chunk doesn't exist anymore " + chunkPosition.ToString());
            }
        }
    }
    else if (msgID == NETWORK_REQUEST_CHUNK_HIT)
    {
        if (network->IsServerRunning())
        {
            const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
            // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
            MemoryBuffer msg(data);
            Vector3 chunkPosition = msg.ReadVector3();
            IntVector3 blockPosition = msg.ReadIntVector3();
            auto chunk = GetChunkByPosition(chunkPosition);
            if (chunk)
            {
                chunk->SetBlockData(blockPosition, BT_AIR);
                chunk->MarkForGeometryCalculation();
            }

            VectorBuffer buffer;
            buffer.WriteVector3(chunkPosition);
            buffer.WriteIntVector3(blockPosition);
            buffer.WriteInt(static_cast<int>(BT_AIR));
            network->BroadcastMessage(NETWORK_SEND_CHUNK_UPDATE, true, true, buffer);
        }
    }
    else if (msgID == NETWORK_REQUEST_CHUNK_ADD)
    {
        if (network->IsServerRunning())
        {
            const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
            // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
            MemoryBuffer msg(data);
            Vector3 chunkPosition = msg.ReadVector3();
            IntVector3 blockPosition = msg.ReadIntVector3();
            BlockType type = static_cast<BlockType>(msg.ReadInt());
            auto chunk = GetChunkByPosition(chunkPosition);
            if (chunk)
            {
                chunk->SetBlockData(blockPosition, type);
                //                chunk->MarkForGeometryCalculation();
            }
            VectorBuffer buffer;
            buffer.WriteVector3(chunkPosition);
            buffer.WriteIntVector3(blockPosition);
            buffer.WriteInt(static_cast<int>(type));
            network->BroadcastMessage(NETWORK_SEND_CHUNK_UPDATE, true, true, buffer);
        }
    }
    else if (msgID == NETWORK_SEND_CHUNK_UPDATE)
    {
        if (!network->IsServerRunning())
        {
            const PODVector<unsigned char>& data = eventData[P_DATA].GetBuffer();
            // Use a MemoryBuffer to read the message data so that there is no unnecessary copying
            MemoryBuffer msg(data);
            Vector3 chunkPosition = msg.ReadVector3();
            IntVector3 blockPosition = msg.ReadIntVector3();
            BlockType type = static_cast<BlockType>(msg.ReadInt());
            auto chunk = GetChunkByPosition(chunkPosition);
            if (chunk)
            {
                chunk->SetBlockData(blockPosition, type);
                //                chunk->MarkForGeometryCalculation();
            }
        }
    }
}
    #endif

void VoxelWorld::SetSunlight(float value)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto waterMaterial = cache->GetResource<Material>("Materials/VoxelWater.xml");
    auto landMaterial = cache->GetResource<Material>("Materials/Voxel.xml");
    waterMaterial->SetShaderParameter("SunlightIntensity", value);
    landMaterial->SetShaderParameter("SunlightIntensity", value);
}
#endif
