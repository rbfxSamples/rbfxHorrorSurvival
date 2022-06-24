#pragma once

#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>
#include "../BaseWindow.h"

class NewGameSettingsWindow : public BaseWindow
{
	URHO3D_OBJECT(NewGameSettingsWindow, BaseWindow);

public:
	NewGameSettingsWindow(Context* context);

	virtual ~NewGameSettingsWindow();

	virtual void Init() override;

protected:

	virtual void Create() override;

private:

	void SubscribeToEvents();

	Button* CreateButton(UIElement* parent, const ea::string& text, int width, IntVector2 position);
	CheckBox* CreateCheckbox(const ea::string& label);

	void CreateLevelSelection();

	SharedPtr<Window> baseWindow_;
	SharedPtr<UIElement> levelSelection_;
	SharedPtr<CheckBox> startServer_;
	SharedPtr<CheckBox> connectServer_;
};
