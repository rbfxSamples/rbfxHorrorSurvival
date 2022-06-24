#pragma once

#include "SingleAchievement.h"
#include <Urho3D/Container/Str.h>
#include <Urho3D/Core/WorkQueue.h>

using namespace Urho3D;

struct AchievementRule
{
    /**
     * Name of the event which should trigger achievement check
     */
    ea::string eventName;
    /**
     * Image path
     */
    ea::string image;
    /**
     * Message to display when this achievement is unlocked
     */
    ea::string message;
    /**
     * How many times the event should be called till the achievement can be marked as unlocked
     */
    int threshold;
    /**
     * Counter how much times the event was called - when this is equal to threshold, achievement is unlocked
     */
    int current;
    /**
     * Whether achievement was displayed or not
     */
    bool completed;
    /**
     * Optional - event parameter to check and decide if the counter should be incremented or not
     */
    ea::string parameterName;
    /**
     * Optional - event parameter value which value should match to allow counter incrementing
     */
    Variant parameterValue;
    /**
     * How to check if the achievement criteria was met
     * false - check only incoming event
     * true - check `eventData` object and compare `parameterName` and `parameterValue`
     */
    bool deepCheck;
};

class Achievements : public Object
{
    URHO3D_OBJECT(Achievements, Object);

public:
    Achievements(Context* context);

    virtual ~Achievements();

    /**
     * Enable/Disable achievements from showing
     * Achievements won't be lost but their displaying will be added
     * to the queue when this is disabled
     */
    void SetShowAchievements(bool show);

    /**
     * Get all registered achievements
     */
    ea::list<AchievementRule> GetAchievements();

    /**
     * Clear current progress of achievements
     */
    void ClearAchievementsProgress();

private:
    friend void SaveProgressAsync(const WorkItem* item, unsigned threadIndex);

    /**
     * Initialize achievements
     */
    void Init();

    /**
     * Subscribe to the notification events
     */
    void SubscribeToEvents();

    /**
     * Create or add new event to the queue
     */
    void HandleNewAchievement(StringHash eventType, VariantMap& eventData);

    void HandleAddAchievement(StringHash eventType, VariantMap& eventData);

    void AddAchievement(ea::string message, ea::string eventName, ea::string image, int threshold,
        ea::string parameterName = "", Variant parameterValue = Variant::EMPTY);

    /**
     * Update SingleAchievement statuses
     */
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /**
     * Handle registered event statuses
     */
    void HandleRegisteredEvent(StringHash eventType, VariantMap& eventData);

    /**
     * Load achievements configuration
     */
    void LoadAchievementList();

    /**
     * Save achievement progress
     */
    void SaveProgress();

    /**
     * Load achievement progress
     */
    void LoadProgress();

    /**
     * Calculate the total count of achievements
     */
    int CountAchievements();

    /**
     * Active achievement list
     */
    ea::list<SharedPtr<SingleAchievement>> activeAchievements_;

    /**
     * Only 1 achievement is allowed at a time,
     * so all additional achievements are added to the queue
     */
    ea::list<VariantMap> achievementQueue_;

    /**
     * Disable achievements from showing, all incoming achievements
     * will be added to the queue. This might be useful to avoid showing achievements
     * on Splash or Credits screens for example
     */
    bool showAchievements_;

    /**
     * All registered achievements
     */
    ea::hash_map<StringHash, ea::list<AchievementRule>> registeredAchievements_;

    /**
     * All achievements
     */
    ea::list<AchievementRule> achievements_;

    /**
     * Current achievement progress
     */
    ea::hash_map<ea::string, int> progress_;
};