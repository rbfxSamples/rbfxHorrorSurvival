#ifdef VOXEL_SUPPORT
    #pragma once
    #include "../../Generator/PerlinNoise.h"
    #include "../../Generator/SimplexNoise.h"
    #include "VoxelDefs.h"
    #include <Urho3D/Core/Object.h>

using namespace Urho3D;

class ChunkGenerator : public Object
{
    URHO3D_OBJECT(ChunkGenerator, Object);
    ChunkGenerator(Context* context);
    virtual ~ChunkGenerator();

public:
    static void RegisterObject(Context* context);
    int GetTerrainHeight(const Vector3& blockPosition);
    BlockType GetBlockType(const Vector3& blockPosition, int surfaceHeight);
    BlockType GetCaveBlockType(const Vector3& blockPosition, BlockType currentBlock);
    bool HaveTree(const Vector3& blockPosition);
    Biome GetBiomeType(const Vector3& blockPosition);
    void SetSeed(int seed);

private:
    PerlinNoise perlin_;
    SimplexNoise simplexNoise_;
};
#endif
