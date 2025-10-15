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

private:

	Gleam::EntityHandle mEntity;
};

} // namespace Gleam
