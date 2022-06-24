#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

struct LoadingStep
{
    ea::string event;
    ea::string name;
    bool finished;
    bool ack;
    bool ackSent;
    Timer ackTimer;
    float progress;
    Timer loadTime;
    bool failed;
    bool autoRemove;
    StringVector dependsOn;
    ea::string map;
};

struct MapInfo
{
    ea::string map;
    ea::string name;
    ea::string description;
    ea::string image;
    StringVector commands;
    Vector3 startPoint;
    ea::string startNode;
};

class SceneManager : public Object
{
    URHO3D_OBJECT(SceneManager, Object);

public:
    SceneManager(Context* context);

    ~SceneManager();

    static void RegisterObject(Context* context);

    /**
     * Start loading scene from the file
     */
    void LoadScene(const ea::string& filename);

    /**
     * Currently active scene
     */
    SharedPtr<Scene>& GetActiveScene() { return activeScene_; }

    /**
     * Scene loading progress
     */
    float GetProgress() { return progress_; }

    /**
     * Current loading status
     */
    const ea::string& GetStatusMessage() const { return loadingStatus_; }

    /**
     * Set current progress to 0
     */
    void ResetProgress();

    void CleanupScene();

    const ea::vector<MapInfo>& GetAvailableMaps() const;

    const MapInfo* GetCurrentMapInfo() const;

private:
    void CleanupLoadingSteps();

    void LoadDefaultMaps();

    bool CanLoadingStepRun(LoadingStep& loadingStep);

    /**
     * Scene loading in progress
     */
    void HandleAsyncSceneLoadingProgress(StringHash eventType, VariantMap& eventData);

    /**
     * Scene loading finished
     */
    void HandleAsyncSceneLoadingFinished(StringHash eventType, VariantMap& eventData);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /**
     * Add new loading step to the loading screen
     */
    void HandleRegisterLoadingStep(StringHash eventType, VariantMap& eventData);

    /**
     * Ackownledge the loading step and let it finish
     */
    void HandleLoadingStepAck(StringHash eventType, VariantMap& eventData);

    /**
     * Receive update status for specific loading step
     */
    void HandleLoadingStepProgress(StringHash eventType, VariantMap& eventData);

    /**
     * Receive loading step finished message
     */
    void HandleLoadingStepFinished(StringHash eventType, VariantMap& eventData);

    /**
     * Receive loading step should be fixed message
     */
    void HandleSkipLoadingStep(StringHash eventType, VariantMap& eventData);

    /**
     * Add map to the map selection
     */
    void HandleAddMap(StringHash eventType, VariantMap& eventData);

    MapInfo* GetMap(const ea::string& filename);

    /**
     * Current active scene
     */
    SharedPtr<Scene> activeScene_;

    /**
     * Current progress
     */
    float progress_{0.0f};

    /**
     * Target progress, `progress` variable will reach `targetProgress` in few seconds
     */
    float targetProgress_{0.0f};

    /**
     * Current loading status
     */
    ea::string loadingStatus_;

    /**
     * ea::list of all the loading steps registered in the system
     */
    ea::hash_map<StringHash, LoadingStep> loadingSteps_;

    ea::vector<MapInfo> availableMaps_;

    MapInfo* currentMap_;
};
