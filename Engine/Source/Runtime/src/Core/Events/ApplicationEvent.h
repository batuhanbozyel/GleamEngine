#pragma once
#include "Event.h"

namespace Gleam {

class AppCloseEvent : public Event
{
public:

	AppCloseEvent() = default;

};

class AppTickEvent : public Event
{
public:

	AppTickEvent() = default;

};

class AppUpdateEvent : public Event
{
public:

	AppUpdateEvent() = default;

};

class AppRenderEvent : public Event
{
public:

	AppRenderEvent() = default;

};

} // namespace Gleam