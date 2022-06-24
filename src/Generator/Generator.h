#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/UI/Text.h>

using namespace Urho3D;

class Generator : public Object
{
    URHO3D_OBJECT(Generator, Object);

public:
    Generator(Context* context);

    virtual ~Generator();

    Image* GenerateImage(double frequency, int octaves, int seed);

    Image* GetImage() const { return generatedImage_; };

    void GenerateTextures();

    void Save();

private:
    /**
     * Subscribe to notification events
     */
    void SubscribeToEvents();

    SharedPtr<Image> generatedImage_;
};
