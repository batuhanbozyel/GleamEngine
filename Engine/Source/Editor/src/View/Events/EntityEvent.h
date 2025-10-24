#pragma once
#include "Core/Events/Event.h"
#include "World/Entity.h"

namespace GEditor {

class EntitySelectedEvent : public Gleam::Event
{
public:

	EntitySelectedEvent(Gleam::EntityHandle entity)
		: mEntity(entity) {}

	Gleam::EntityHandle GetEntity() const
	{
		return mEntity;
	}

	virtual Gleam::TString ToString() const override
	{
		Gleam::TStringStream ss;
		ss << "EntitySelectedEvent: " << (uint32_t)mEntity;
		return ss.str();
	}

private:

	Gleam::EntityHandle mEntity;
};

} // namespace Gleam
