#pragma once
#include "InputDevice.h"
#include "Math/Vector2.h"

#include <SDL3/SDL_gamepad.h>

struct SDL_GamepadButtonEvent;
struct SDL_GamepadAxisEvent;

namespace Gleam {

GENUM(GamepadControl, "92789F63-C67C-421B-AD1B-1306D4F86ADF", Serializable)
{
	GITEM(South, "5C5AE207-4A50-498C-91FB-61840495FF52", PrettyName("Button South")),
	GITEM(East, "51412969-0E07-486E-9ADD-3497A2874E62", PrettyName("Button East")),
	GITEM(West, "C03EC4A1-0CCB-44A1-A874-19B70E90E4F2", PrettyName("Button West")),
	GITEM(North, "F895B1A1-4984-4464-8976-B29314CEB47A", PrettyName("Button North")),
	GITEM(LeftShoulder, "91D1878B-252D-49D3-A20D-832171744B2F", PrettyName("Left Shoulder")),
	GITEM(RightShoulder, "28C2689D-DFB8-4D8E-B102-ABE39AAA81BB", PrettyName("Right Shoulder")),
	GITEM(LeftStickPress, "1B169959-5825-40E3-AD7D-3D2BDB26F507", PrettyName("Left Stick Press")),
	GITEM(RightStickPress, "F983E564-0AA2-415B-A50C-60F2BAEFDF18", PrettyName("Right Stick Press")),
	GITEM(DPadUp, "24FED4C0-BFD3-41D6-AF65-CFC2FF4D4198", PrettyName("D-Pad Up")),
	GITEM(DPadDown, "D7F1F4D4-66AE-45CB-8CA9-3216E8A10A5C", PrettyName("D-Pad Down")),
	GITEM(DPadLeft, "0B98A15F-AD71-46A4-A746-914AC143D5F4", PrettyName("D-Pad Left")),
	GITEM(DPadRight, "69F73A68-43FF-497D-B0E1-A10E875691D6", PrettyName("D-Pad Right")),
	GITEM(Start, "D707E2DC-9D01-4C3A-8E6E-D2875B19969D", PrettyName("Start")),
	GITEM(Select, "85E4E5E9-6D9F-488D-BA64-144C5D498E4C", PrettyName("Select")),
	GITEM(Guide, "509F5432-12DD-480D-8744-04E9DE0903CE", PrettyName("Guide")),
	GITEM(LeftTrigger, "AB2BEAAA-50C4-405C-BE9A-D0F91F689CF9", PrettyName("Left Trigger")),
	GITEM(RightTrigger, "A5711693-B481-40AA-880D-39FEA48A4BA4", PrettyName("Right Trigger")),
	GITEM(LeftStick, "3A1F421F-D61E-4903-88B8-6B38A648637D", PrettyName("Left Stick")),
	GITEM(RightStick, "B9389795-E3EB-4583-8591-B387B4BE9007", PrettyName("Right Stick")),
	GITEM(LeftStickX, "33160927-9DEE-4584-A48B-55D6F0847EA8", PrettyName("Left Stick X")),
	GITEM(LeftStickY, "C5D56773-67D7-4E66-8DCF-242351408FF9", PrettyName("Left Stick Y")),
	GITEM(RightStickX, "23749242-6308-4E8C-BD00-B2BD22B06F76", PrettyName("Right Stick X")),
	GITEM(RightStickY, "6284FC54-5092-4C07-BF6A-602F6B63077D", PrettyName("Right Stick Y"))
};

class Gamepad final : public InputDevice
{
	friend class InputSystem;
public:

	static constexpr uint32_t ButtonCount = static_cast<uint32_t>(GamepadControl::RightTrigger) + 1;

	static constexpr float TriggerPressPoint = 0.5f;

	Gamepad(uint32_t id, SDL_Gamepad* handle);

	bool IsPressed(GamepadControl button) const;

	bool WasPressedThisFrame(GamepadControl button) const;

	bool WasReleasedThisFrame(GamepadControl button) const;

	Float2 GetLeftStick() const;

	Float2 GetRightStick() const;

	float GetLeftTrigger() const;

	float GetRightTrigger() const;

	void SetRumble(float lowFrequency, float highFrequency, uint32_t durationMs) const;

	virtual Float3 ReadControl(uint32_t code) const override;

	virtual bool WasControlPressedThisFrame(uint32_t code) const override;

	virtual bool WasControlReleasedThisFrame(uint32_t code) const override;

	virtual uint64_t GetControlPressTime(uint32_t code) const override;

	virtual uint64_t GetControlReleaseTime(uint32_t code) const override;

private:

	virtual void Commit() override;

	void Connect(SDL_Gamepad* handle);

	void Disconnect();

	bool Matches(const SDL_GUID& guid) const;

	SDL_JoystickID GetJoystickId() const;

	void ProcessButtonEvent(const SDL_GamepadButtonEvent& buttonEvent);

	void ProcessAxisEvent(const SDL_GamepadAxisEvent& axisEvent);

	void UpdateTriggerButton(GamepadControl trigger, float value, uint64_t timestamp);

	InputButtonStates<ButtonCount> mButtons;

	TArray<float, SDL_GAMEPAD_AXIS_COUNT> mPendingAxes{};

	TArray<float, SDL_GAMEPAD_AXIS_COUNT> mAxes{};

	SDL_Gamepad* mHandle = nullptr;

	SDL_JoystickID mJoystickId = 0;

	SDL_GUID mGuid{};

};

} // namespace Gleam
