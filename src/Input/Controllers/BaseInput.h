#pragma once

#include <Urho3D/Core/Object.h>
#include "../../Config/ConfigFile.h"

using namespace Urho3D;

class BaseInput : public Object
{
	URHO3D_OBJECT(BaseInput, Object);

public:
	BaseInput(Context* context) :
		Object(context),
		activeAction_(0),
		sensitivityX_(0.1f),
		sensitivityY_(0.1f),
		deadzone_(0.1f),
		invertX_(false),
		invertY_(false)
	{
	}

	virtual ~BaseInput() {};
	virtual void Show() {};
	virtual void Hide() {};

	virtual void LoadConfig() {};

	/**
	 * Get name of the key which uses specific action
	 */
	virtual ea::string GetActionKeyName(int action) { return ea::string::EMPTY; };

	/**
	 * Bind key to specific action
	 */
	void SetKeyToAction(int key, int action) {
		mappedKeyToControl_[key] = action;
		mappedControlToKey_[action] = key;
	}

	bool IsActionUsed(int action) {
		return mappedControlToKey_.contains(action);
	}

	/**
	 * Get all action mapping against controls
	 */
	ea::hash_map<int, int> GetConfigMap()
	{
		return mappedControlToKey_;
	}

	/**
	 * Start the key to action mapping process
	 */
	void StartMappingAction(int action)
	{
		activeAction_ = action;
	}

	/**
	 * Stop the key to action mapping process
	 */
	void StopMappingAction()
	{
		activeAction_ = 0;
		timer_.Reset();
	}

	/**
	 * Release all mappings for specific action
	 */
	void ReleaseAction(int action)
	{
		for (auto it = mappedControlToKey_.begin(); it != mappedControlToKey_.end(); ++it) {
			int keyCode = (*it).second;
			int actionCode = (*it).first;
			if (action == actionCode) {
				mappedControlToKey_.Erase(actionCode);
				mappedKeyToControl_.Erase(keyCode);
			}
		}
		for (auto it = mappedKeyToControl_.begin(); it != mappedKeyToControl_.end(); ++it) {
			int keyCode = (*it).first;
			int actionCode = (*it).second;
			if (action == actionCode) {
				mappedKeyToControl_.Erase(keyCode);
				mappedControlToKey_.Erase(actionCode);
			}
		}
	}

	/**
	 * Release all mappings for specific key
	 */
	void ReleaseKey(int key)
	{
		for (auto it = mappedKeyToControl_.begin(); it != mappedKeyToControl_.end(); ++it) {
			int keyCode = (*it).first;
			int actionCode = (*it).second;
			if (key == keyCode) {
				mappedKeyToControl_.Erase(keyCode);
				mappedControlToKey_.Erase(actionCode);
			}
		}
		for (auto it = mappedControlToKey_.begin(); it != mappedControlToKey_.end(); ++it) {
			int keyCode = (*it).second;
			int actionCode = (*it).first;
			if (key == keyCode) {
				mappedControlToKey_.Erase(actionCode);
				mappedKeyToControl_.Erase(keyCode);
			}
		}
	}

	void SetInvertX(bool enabled)
	{
		invertX_ = enabled;
	}

	bool GetInvertX()
	{
		return invertX_;
	}

	void SetInvertY(bool enabled)
	{
		invertY_ = enabled;
	}

	bool GetInvertY()
	{
		return invertY_;
	}

	void SetSensitivityX(float value)
	{
		if (value < minSensitivity_) {
			value = minSensitivity_;
		}
		sensitivityX_ = value;
	}

	float GetSensitivityX()
	{
		return sensitivityX_;
	}

	void SetSensitivityY(float value)
	{
		if (value < minSensitivity_) {
			value = minSensitivity_;
		}
		sensitivityY_ = value;
	}

	float GetSensitivityY()
	{
		return sensitivityY_;
	}

	void SetDeadzone(float value)
	{
		deadzone_ = value;
	}

	float GetDeadzone()
	{
		return deadzone_;
	}

protected:

	void SetMinSensitivity(float value)
	{
		minSensitivity_ = value;
	}

	// Control against key map
	ea::hash_map<int, int> mappedControlToKey_;
	// key against control map
	ea::hash_map<int, int> mappedKeyToControl_;

	int activeAction_;

	Timer timer_;

	bool invertX_;
	bool invertY_;
	float sensitivityX_;
	float sensitivityY_;
	float minSensitivity_;
	float deadzone_;
};
