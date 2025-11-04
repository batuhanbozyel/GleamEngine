#pragma once
#include "Event.h"

namespace Gleam {

class AppCloseEvent : public Event
{
public:

	AppCloseEvent() = default;

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "AppCloseEvent";
		return ss.str();
	}

};

class AppTickEvent : public Event
{
public:

	AppTickEvent() = default;

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "AppTickEvent";
		return ss.str();
	}

};

class AppUpdateEvent : public Event
{
public:

	AppUpdateEvent() = default;

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "AppUpdateEvent";
		return ss.str();
	}

};

class AppRenderEvent : public Event
{
public:

	AppRenderEvent() = default;

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "AppRenderEvent";
		return ss.str();
	}

};

} // namespace Gleam