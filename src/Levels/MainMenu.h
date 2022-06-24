#pragma once

#include <Urho3D/UI/Button.h>
#include "../BaseLevel.h"
#include <vector>

using namespace Urho3D;

namespace Levels {

    class MainMenu : public BaseLevel
    {
    URHO3D_OBJECT(MainMenu, BaseLevel);

    public:
        MainMenu(Context* context);
        ~MainMenu();
        static void RegisterObject(Context* context);

    protected:
        void Init () override;

    private:
        void CreateScene();

        void CreateUI();

        void SubscribeToEvents();

        void AddButton(const ea::string& buttonName, const ea::string& label, const ea::string& name, const ea::string& eventToCall);

        void HandleUpdate(StringHash eventType, VariantMap& eventData);

        void InitCamera();

        SharedPtr<Node> cameraRotateNode_;
        SharedPtr<UIElement> buttonsContainer_;
        ea::list<SharedPtr<Button>> dynamicButtons_;

        Button* CreateButton(const ea::string& text);
    };
}
