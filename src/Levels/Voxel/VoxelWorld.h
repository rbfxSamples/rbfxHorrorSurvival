#ifdef VOXEL_SUPPORT
#pragma once
#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/ea::list.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Core/Timer.h>
#include <queue>
#include <map>

#include "Chunk.h"

struct ChunkNode {
	ChunkNode(Vector3 position, int distance) : position_(position), distance_(distance) {}
	Vector3 position_;
	int distance_;
};

class VoxelWorld : public Object {
	URHO3D_OBJECT(VoxelWorld, Object);
	VoxelWorld(Context* context);

	static void RegisterObject(Context* context);
	friend void UpdateChunkState(const WorkItem* item, unsigned threadIndex);

	void AddObserver(SharedPtr<Node> observer);
	void RemoveObserver(SharedPtr<Node> observer);
	Chunk* GetChunkByPosition(const Vector3& position);
	void RemoveBlockAtPosition(const Vector3& position);
	VoxelBlock* GetBlockAt(Vector3 position);
	void Init();
	bool IsChunkValid(Chunk* chunk);
	const ea::string GetBlockName(BlockType type);
	Vector3 GetWorldToChunkPosition(const Vector3& position);
	IntVector3 GetWorldToChunkBlockPosition(const Vector3& position);
private:
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	void HandleChunkReceived(StringHash eventType, VariantMap& eventData);
	void HandleWorkItemFinished(StringHash eventType, VariantMap& eventData);
	void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	void LoadChunk(const Vector3& position);
	void UpdateChunks();
	Vector3 GetNodeToChunkPosition(Node* node);
	bool IsChunkLoaded(const Vector3& position);
	bool IsEqualPositions(Vector3 a, Vector3 b);
	Chunk* CreateChunk(const Vector3& position);
	ea::string GetChunkIdentificator(const Vector3& position);
	bool ProcessQueue();
	void AddChunkToQueue(Vector3 position, int distance = 0);
	void SetSunlight(float value);

	//    void RaycastFromObservers();

	//    ea::list<SharedPtr<Chunk>> chunks_;
	ea::list<WeakPtr<Node>> observers_;
	Scene* scene_;
	ea::list<Vector3> removeBlocks_;
	ea::hash_map<ea::string, SharedPtr<Chunk>> chunks_;
	Mutex mutex_;
	SharedPtr<WorkItem> updateWorkItem_;
	bool reloadAllChunks_{ false };
	Timer sunlightTimer_;
	std::queue<ChunkNode> chunkBfsQueue_;
	ea::hash_map<Vector3, int> chunksToLoad_;
	Timer updateTimer_;
	int visibleDistance_{ 5 };
};
#endif
