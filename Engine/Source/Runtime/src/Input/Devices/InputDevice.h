#pragma once
#include "Core/Attributes.h"
#include "Core/GUID.h"
#include "Container/Array.h"
#include "Container/String.h"
#include "Math/Vector3.h"

namespace Gleam {

GENUM(InputDeviceType, "0DF9E937-A571-487C-9B7E-DC7A3489AE1C", Serializable)
{
	GITEM(Keyboard, "3C9FCF81-F8E7-4725-B450-D2735B560FB2"),
	GITEM(Mouse, "1C436CD2-9B42-4663-8A49-009079355FAF"),
	GITEM(Gamepad, "C88C7772-5C40-45D1-ACDA-DE499EDB008F")
};

struct InputButton
{
	bool down = false;
	uint8_t pressCount = 0;
	uint8_t releaseCount = 0;
	uint64_t pressTime = 0;
	uint64_t releaseTime = 0;
};

template<size_t N>
class InputButtonStates
{
public:

	void Press(uint32_t index, uint64_t timestamp)
	{
		auto& button = mPending[index];
		if (button.down == false)
		{
			button.down = true;
			if (button.pressCount == 0)
			{
				button.pressTime = timestamp;
			}
			++button.pressCount;
		}
	}

	void Release(uint32_t index, uint64_t timestamp)
	{
		auto& button = mPending[index];
		if (button.down)
		{
			button.down = false;
			button.releaseTime = timestamp;
			++button.releaseCount;
		}
	}

	void ReleaseAll(uint64_t timestamp)
	{
		for (uint32_t i = 0; i < N; ++i)
		{
			Release(i, timestamp);
		}
	}

	void Commit()
	{
		mCurrent = mPending;
		for (auto& button : mPending)
		{
			button.pressCount = 0;
			button.releaseCount = 0;
		}
	}

	bool IsPressed(uint32_t index) const
	{
		return mCurrent[index].down;
	}

	bool WasPressedThisFrame(uint32_t index) const
	{
		return mCurrent[index].pressCount > 0;
	}

	bool WasReleasedThisFrame(uint32_t index) const
	{
		return mCurrent[index].releaseCount > 0;
	}

	uint64_t GetPressTime(uint32_t index) const
	{
		return mCurrent[index].pressTime;
	}

	uint64_t GetReleaseTime(uint32_t index) const
	{
		return mCurrent[index].releaseTime;
	}

private:

	TArray<InputButton, N> mPending{};

	TArray<InputButton, N> mCurrent{};

};

class InputDevice
{
	friend class InputSystem;
public:

	InputDevice(InputDeviceType type, uint32_t id, const char* name);

	virtual ~InputDevice() = default;

	InputDeviceType GetType() const;

	uint32_t GetId() const;

	const TString& GetName() const;

	bool IsConnected() const;

	virtual Float3 ReadControl(uint32_t code) const = 0;

	virtual bool WasControlPressedThisFrame(uint32_t code) const = 0;

	virtual bool WasControlReleasedThisFrame(uint32_t code) const = 0;

	// Nanoseconds on the SDL_GetTicksNS clock, valid while the matching WasControl*ThisFrame is true.
	virtual uint64_t GetControlPressTime(uint32_t code) const = 0;

	virtual uint64_t GetControlReleaseTime(uint32_t code) const = 0;

protected:

	virtual void Commit() = 0;

	TString mName;

	bool mConnected = true;

private:

	InputDeviceType mType;

	uint32_t mId;

};

} // namespace Gleam
