#pragma once
#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/List.h>
#include "BaseWindow.h"

using namespace Urho3D;

class WindowManager : public Object
{
    URHO3D_OBJECT(WindowManager, Object);

public:
    WindowManager(Context* context);

    virtual ~WindowManager();

    /**
     * Is specific window is already opened
     */
    bool IsWindowOpen(ea::string windowName);

    bool IsAnyWindowOpened();

private:

    void RegisterAllFactories();

    void SubscribeToEvents();

    /**
     * Create specific window via event
     */
    void HandleOpenWindow(StringHash eventType, VariantMap& eventData);

    /**
     * Prepare window for closing. Adds window to the queue for windows which should
     * be destroyed in the next frame
     */
    void HandleCloseWindow(StringHash eventType, VariantMap& eventData);

    /**
     * Close all active windows
     */
    void HandleCloseAllWindows(StringHash eventType, VariantMap& eventData);

    /**
     * Check whether there are windows which should be closed.
     */
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /**
     * Destroy specific window
     */
    void CloseWindow(ea::string windowName);

    /**
     * List of all active windows objects
     */
    List<SharedPtr<Object>> windowList_;

    /**
     * List of opened windows
     */
    List<ea::string> openedWindows_;

    /**
     * Window list which should be destroyed in the next frame
     */
    List<ea::string> closeQueue_;
};