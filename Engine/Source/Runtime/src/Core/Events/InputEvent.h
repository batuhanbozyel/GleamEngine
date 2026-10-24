#pragma once
#include "Event.h"
#include "Input/InputUser.h"

namespace Gleam {

class InputDeviceEvent : public Event
{
public:

	InputDevice* GetDevice() const { return mDevice; }

protected:

	InputDeviceEvent(InputDevice* device)
		: mDevice(device) {}

	InputDevice* mDevice;
};

class InputDeviceConnectedEvent : public InputDeviceEvent
{
public:

	InputDeviceConnectedEvent(InputDevice* device)
		: InputDeviceEvent(device) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "InputDeviceConnectedEvent: " << mDevice->GetName().c_str();
		return ss.str();
	}
};

class InputDeviceDisconnectedEvent : public InputDeviceEvent
{
public:

	InputDeviceDisconnectedEvent(InputDevice* device)
		: InputDeviceEvent(device) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "InputDeviceDisconnectedEvent: " << mDevice->GetName().c_str();
		return ss.str();
	}
};

class KeymapChangedEvent : public Event
{
public:

	virtual TString ToString() const override
	{
		return "KeymapChangedEvent";
	}
};

class PlayerEvent : public Event
{
public:

	InputUser* GetPlayer() const { return mPlayer; }

protected:

	PlayerEvent(InputUser* player)
		: mPlayer(player) {}

	InputUser* mPlayer;
};

class PlayerJoinedEvent : public PlayerEvent
{
public:

	PlayerJoinedEvent(InputUser* player)
		: PlayerEvent(player) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "PlayerJoinedEvent: " << mPlayer->GetPlayerIndex();
		return ss.str();
	}
};

class PlayerLeftEvent : public PlayerEvent
{
public:

	PlayerLeftEvent(InputUser* player)
		: PlayerEvent(player) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "PlayerLeftEvent: " << mPlayer->GetPlayerIndex();
		return ss.str();
	}
};

} // namespace Gleam
