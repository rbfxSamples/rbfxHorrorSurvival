#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/ea::vector.h>
#include <Urho3D/UI/Text.h>

using namespace Urho3D;

struct NotificationData {
	ea::string message;
	Color color;
};

class Notifications : public Object
{
	URHO3D_OBJECT(Notifications, Object);

public:
	Notifications(Context* context);

	virtual ~Notifications();

private:
	virtual void Init();

	/**
	 * Subscribe to notification events
	 */
	void SubscribeToEvents();

	/**
	 * Handle ShowNotification event
	 */
	void HandleNewNotification(StringHash eventType, VariantMap& eventData);

	void CreateNewNotification(NotificationData data);

	/**
	 * Handle message displaying and animations
	 */
	void HandleUpdate(StringHash eventType, VariantMap& eventData);

	/**
	 * Handle game end event
	 */
	void HandleGameEnd(StringHash eventType, VariantMap& eventData);

	/**
	 * ea::list of all active messages
	 */
	ea::vector<SharedPtr<UIElement>> messages_;
	SharedPtr<ObjectAnimation> notificationAnimation_;
	SharedPtr<ValueAnimation> positionAnimation_;
	SharedPtr<ValueAnimation> opacityAnimation_;
	ea::list<NotificationData> messageQueue_;
	Timer timer_;
};
