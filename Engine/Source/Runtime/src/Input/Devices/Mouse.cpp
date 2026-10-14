#include "gpch.h"
#include "Mouse.h"

#include <SDL3/SDL_events.h>

using namespace Gleam;

Mouse::Mouse(uint32_t id)
	: InputDevice(InputDeviceType::Mouse, id, "Mouse")
{

}

bool Mouse::IsPressed(MouseControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.IsPressed(index);
}

bool Mouse::WasPressedThisFrame(MouseControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.WasPressedThisFrame(index);
}

bool Mouse::WasReleasedThisFrame(MouseControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.WasReleasedThisFrame(index);
}

const Float2& Mouse::GetPosition() const
{
	return mPosition;
}

const Float2& Mouse::GetDelta() const
{
	return mDelta;
}

const Float2& Mouse::GetScroll() const
{
	return mScroll;
}

Float3 Mouse::ReadControl(uint32_t code) const
{
	switch (static_cast<MouseControl>(code))
	{
		case MouseControl::Position: return Float3(mPosition.x, mPosition.y, 0.0f);
		case MouseControl::Delta: return Float3(mDelta.x, mDelta.y, 0.0f);
		case MouseControl::Scroll: return Float3(mScroll.x, mScroll.y, 0.0f);
		case MouseControl::DeltaX: return Float3(mDelta.x, 0.0f, 0.0f);
		case MouseControl::DeltaY: return Float3(mDelta.y, 0.0f, 0.0f);
		case MouseControl::ScrollX: return Float3(mScroll.x, 0.0f, 0.0f);
		case MouseControl::ScrollY: return Float3(mScroll.y, 0.0f, 0.0f);
		default: return Float3(IsPressed(static_cast<MouseControl>(code)) ? 1.0f : 0.0f, 0.0f, 0.0f);
	}
}

bool Mouse::WasControlPressedThisFrame(uint32_t code) const
{
	return code < ButtonCount and mButtons.WasPressedThisFrame(code);
}

bool Mouse::WasControlReleasedThisFrame(uint32_t code) const
{
	return code < ButtonCount and mButtons.WasReleasedThisFrame(code);
}

void Mouse::Commit()
{
	mButtons.Commit();
	mPosition = mPendingPosition;
	mDelta = mPendingDelta;
	mScroll = mPendingScroll;
	mPendingDelta = Float2::zero;
	mPendingScroll = Float2::zero;
}

void Mouse::ProcessMotionEvent(const SDL_MouseMotionEvent& motionEvent)
{
	mPendingPosition = Float2(motionEvent.x, motionEvent.y);
	mPendingDelta += Float2(motionEvent.xrel, motionEvent.yrel);
}

void Mouse::ProcessButtonEvent(const SDL_MouseButtonEvent& buttonEvent)
{
	MouseControl button;
	switch (buttonEvent.button)
	{
		case SDL_BUTTON_LEFT: button = MouseControl::Left; break;
		case SDL_BUTTON_RIGHT: button = MouseControl::Right; break;
		case SDL_BUTTON_MIDDLE: button = MouseControl::Middle; break;
		case SDL_BUTTON_X1: button = MouseControl::Back; break;
		case SDL_BUTTON_X2: button = MouseControl::Forward; break;
		default: return;
	}

	const auto index = static_cast<uint32_t>(button);
	if (buttonEvent.down)
	{
		mButtons.Press(index);
	}
	else
	{
		mButtons.Release(index);
	}
}

void Mouse::ProcessWheelEvent(const SDL_MouseWheelEvent& wheelEvent)
{
	mPendingScroll += Float2(wheelEvent.x, wheelEvent.y);
}
