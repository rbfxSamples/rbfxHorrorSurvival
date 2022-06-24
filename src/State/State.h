#pragma once

#include <Urho3D/Container/Str.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Resource/JSONFile.h>

using namespace Urho3D;

class State : public Object
{
    URHO3D_OBJECT(State, Object);

    static void RegisterObject(Context* context);

public:
    State(Context* context);

    virtual ~State();

    void SetValue(const ea::string& name, const Variant& value, bool save = false);
    const Variant& GetValue(const ea::string& name);

private:
    void SubscribeToEvents();
    void Load();
    void Save();
    void HandleSetParameter(StringHash eventType, VariantMap& eventData);
    void HandleIncrementParameter(StringHash eventType, VariantMap& eventData);
    ea::string fileLocation_;

    VariantMap data_;
};
