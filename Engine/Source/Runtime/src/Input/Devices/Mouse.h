#pragma once
#include "InputDevice.h"
#include "Math/Vector2.h"

struct SDL_MouseMotionEvent;
struct SDL_MouseButtonEvent;
struct SDL_MouseWheelEvent;

namespace Gleam {

GENUM(MouseControl, "9712DE1B-1EF6-4A7F-B19C-3D7E85C4F43C", Serializable)
{
	GITEM(Left, "32F2B92E-31C3-48DF-9554-2AF593CA25D2", PrettyName("Left Button")),
	GITEM(Right, "CFE45FFB-4989-43B8-820A-AC180D032590", PrettyName("Right Button")),
	GITEM(Middle, "A506AFFD-C0C5-4868-B9B2-5547CE16A052", PrettyName("Middle Button")),
	GITEM(Back, "0CDA87C5-5AD5-47D3-97BF-883C1FFC78EE", PrettyName("Back Button")),
	GITEM(Forward, "F024ADB6-3E4B-4038-B0DA-CBD99D9ECFE5", PrettyName("Forward Button")),
	GITEM(Position, "32A36AD4-256C-4A65-ABB6-45B801C5D318", PrettyName("Position")),
	GITEM(Delta, "E1048562-B2C6-4547-86A5-F09EF0DB58E6", PrettyName("Delta")),
	GITEM(Scroll, "A972A876-63BD-44EA-9FDA-8849DE6BA281", PrettyName("Scroll")),
	GITEM(DeltaX, "5FEBC418-683C-4042-8194-BD5E623CA761", PrettyName("Delta X")),
	GITEM(DeltaY, "14F999B6-E3EC-4B28-BCF7-2126DD71A933", PrettyName("Delta Y")),
	GITEM(ScrollX, "258BAF99-170B-4F80-8BC0-631CADF670D3", PrettyName("Scroll X")),
	GITEM(ScrollY, "343F8B82-1303-4CDB-9788-7B2A2D7C4F8E", PrettyName("Scroll Y"))
};

class Mouse final : public InputDevice
{
	friend class InputSystem;
public:

	static constexpr uint32_t ButtonCount = static_cast<uint32_t>(MouseControl::Forward) + 1;

	Mouse(uint32_t id);

	bool IsPressed(MouseControl button) const;

	bool WasPressedThisFrame(MouseControl button) const;

	bool WasReleasedThisFrame(MouseControl button) const;

	const Float2& GetPosition() const;

	const Float2& GetDelta() const;

	const Float2& GetScroll() const;

	virtual Float3 ReadControl(uint32_t code) const override;

	virtual bool WasControlPressedThisFrame(uint32_t code) const override;

	virtual bool WasControlReleasedThisFrame(uint32_t code) const override;

private:

	virtual void Commit() override;

	void ProcessMotionEvent(const SDL_MouseMotionEvent& motionEvent);

	void ProcessButtonEvent(const SDL_MouseButtonEvent& buttonEvent);

	void ProcessWheelEvent(const SDL_MouseWheelEvent& wheelEvent);

	InputButtonStates<ButtonCount> mButtons;

	Float2 mPendingPosition = Float2::zero;

	Float2 mPendingDelta = Float2::zero;

	Float2 mPendingScroll = Float2::zero;

	Float2 mPosition = Float2::zero;

	Float2 mDelta = Float2::zero;

	Float2 mScroll = Float2::zero;

};

} // namespace Gleam
