#pragma once
#include "Gleam.h"

namespace GEditor {

class EntitySelectedEvent : public Gleam::Event
{
public:

	EntitySelectedEvent(Gleam::Entity entity)
		: mEntity(entity) {}

	Gleam::Entity GetEntity() const
	{
		return mEntity;
	}

private:

	Gleam::Entity mEntity;
};

} // namespace Gleam
