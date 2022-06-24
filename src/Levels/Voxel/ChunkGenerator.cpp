#ifdef VOXEL_SUPPORT
#include <Urho3D/Core/Context.h>
#include "ChunkGenerator.h"
#include "Chunk.h"
#include <Urho3D/IO/Log.h>

ChunkGenerator::ChunkGenerator(Context* context):
Object(context),
simplexNoise_()
{
}

ChunkGenerator::~ChunkGenerator()
{
}

void ChunkGenerator::RegisterObject(Context* context)
{
    context->RegisterFactory<ChunkGenerator>();
}

void ChunkGenerator::SetSeed(int seed)
{
    URHO3D_LOGINFOF("Changing world generating seed to %d", seed);
    perlin_.reseed(seed);
    simplexNoise_.SetSeed(seed);
}

int ChunkGenerator::GetTerrainHeight(const Vector3& blockPosition)
{
//    float octaves = 16;

    float smoothness1 = 55.33f;
    float smoothness2 = 111.33f;
    float smoothness3 = 193.33f;
    double dx1 = blockPosition.x_ / smoothness1;
    double dz1 = blockPosition.z_ / smoothness1;
    double dx2 = blockPosition.x_ / smoothness2;
    double dz2 = blockPosition.z_ / smoothness2;
    double dx3 = blockPosition.x_ / smoothness3;
    double dz3 = blockPosition.z_ / smoothness3;
//    double result1 = perlin_.octaveNoise(dx1, dz1, 1) * 0.5 + 0.5;
    int octaves = (perlin_.octaveNoise(dx2, dz2, 1) * 0.5 + 0.5 + 1) * 16;
    int heightLimit = 100;
    int height = (perlin_.octaveNoise(dx3, dz3, 1) * 0.5 + 0.5) * heightLimit;
//
//
    float smoothness4 = 333.33f;
    double dx4 = blockPosition.x_ / smoothness4;
    double dz4 = blockPosition.z_ / smoothness4;
    double result2 = perlin_.octaveNoise(dx4, dz4, octaves);
//    float result2 = simplexNoise_.noise(dx1, dz1);
//    float height = simplexNoise_.noise(dx2, dz2) * 100;
    float surfaceHeight = result2 * height;
//    if (surfaceHeight < -10) {
//        surfaceHeight = -10;
//    }
    return Ceil(surfaceHeight);
}

bool ChunkGenerator::HaveTree(const Vector3& blockPosition)
{
    float smoothness = 3.13f;
    double dx = blockPosition.x_ / smoothness;
    double dz = blockPosition.z_ / smoothness;
    float result = simplexNoise_.fractal(4, dx, dz) * 0.5 + 0.5;

    if (result > 0.8f) {
        return true;
    }

    return false;
}

Biome ChunkGenerator::GetBiomeType(const Vector3& blockPosition)
{
    float smoothness = 135.12f;
    double dx = blockPosition.x_ / smoothness;
    double dz = blockPosition.z_ / smoothness;

    float result = perlin_.octaveNoise0_1(dx, dz, 4);
//    float threshold = 1.0f / B_NONE;
//    for (int i = 0; i < B_NONE; i++) {
//        if (result <= threshold * (i + 1)) {
//            return static_cast<Biome>(i);
//        }
//    }

    return B_GRASS;
}

BlockType ChunkGenerator::GetBlockType(const Vector3& blockPosition, int surfaceHeight)
{
    Biome biome = GetBiomeType(blockPosition);
//    if (surfaceHeight <= -10) {
//        biome = B_SEA;
//    }
    int heightToSurface = surfaceHeight - blockPosition.y_;
    if (heightToSurface < 0) {
        return BlockType::BT_AIR;
    }
    float smoothness1 = 111.13f;
    float smoothness2 = 222.13f;
    float smoothness3 = 333.13f;
    float result1 = perlin_.octaveNoise(blockPosition.x_ / smoothness1, blockPosition.y_ / smoothness1, 1) * 0.5 + 0.5;
    float result2 = perlin_.octaveNoise(blockPosition.y_ / smoothness2, blockPosition.z_ / smoothness2, 1) * 0.5 + 0.5;
    float result3 = perlin_.octaveNoise(blockPosition.x_ / smoothness3, blockPosition.z_ / smoothness3, 1) * 0.5 + 0.5;
    float result = result1 * result2 * result3;
    if (heightToSurface <= 10) {
        if (result > 0.2) {
            return BlockType::BT_COAL;
        }
        if (result > 0.18) {
            return BlockType::BT_STONE;
        }
        if (result > 0.16) {
            return BlockType::BT_SAND;
        }
        switch (biome) {
            case B_GRASS:
                return BlockType::BT_DIRT;
            case B_SEA:
                return BlockType::BT_WATER;
            case B_FOREST:
                return BlockType::BT_WOOD;
            case B_MOUNTAINS:
                return BlockType::BT_STONE;
            case B_DESERT:
                return BlockType::BT_SAND;
            default:
                return BlockType::BT_DIRT;
        }
    }

    if (result > 0.8) {
        return BlockType::BT_COAL;
    }
    return BlockType::BT_STONE;
}

BlockType ChunkGenerator::GetCaveBlockType(const Vector3& blockPosition, BlockType currentBlock)
{
    if (currentBlock == BlockType::BT_AIR) {
        return currentBlock;
    }

    float smoothness1 = 77.13f;
    float smoothness2 = 66.13f;
    float smoothness3 = 55.13f;
    float result1 = perlin_.octaveNoise(blockPosition.x_ / smoothness1, blockPosition.y_ / smoothness1, 6) * 0.5 + 0.5;
    float result2 = perlin_.octaveNoise(blockPosition.y_ / smoothness2, blockPosition.z_ / smoothness2, 6) * 0.5 + 0.5;
    float result3 = perlin_.octaveNoise(blockPosition.x_ / smoothness3, blockPosition.z_ / smoothness3, 6) * 0.5 + 0.5;
    float result = result1 * result2 * result3;
//    float result = simplexNoise_.noise(blockPosition.x_ / smoothness1, blockPosition.y_ / smoothness1, blockPosition.z_ / smoothness1);
//    float result2 = simplexNoise_.noise(blockPosition.z_ / smoothness1, blockPosition.x_ / smoothness1, blockPosition.y_ / smoothness1);

    if (result > 0.2f) {
        return BlockType::BT_AIR;
    }

    return currentBlock;
}
#endif
