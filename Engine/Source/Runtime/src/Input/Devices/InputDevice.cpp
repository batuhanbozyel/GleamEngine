#include "gpch.h"
#include "InputDevice.h"

using namespace Gleam;

InputDevice::InputDevice(InputDeviceType type, uint32_t id, const char* name)
	: mName(name)
	, mType(type)
	, mId(id)
{

}

InputDeviceType InputDevice::GetType() const
{
	return mType;
}

uint32_t InputDevice::GetId() const
{
	return mId;
}

const TString& InputDevice::GetName() const
{
	return mName;
}

bool InputDevice::IsConnected() const
{
	return mConnected;
}
