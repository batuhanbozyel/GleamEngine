//
//  PhysicsVisualization.h
//  Editor
//

#pragma once
#include <Reflection/Macro.h>

#include "Core/Macro.h"
#include "Container/EnumFlag.h"

namespace Gleam {

GENUM(PhysicsVisualizationFlag, "314664BB-3B01-4EF5-977F-A95D06DCE90C", Serializable, PrettyName("Physics Visualization")) : uint32_t
{
	GITEM(Colliders, "B7F9ED6A-A616-48BC-A794-50709AEF5BE5", PrettyName("Colliders")) = BIT(0),
	GITEM(Triggers, "B408253D-4017-434A-A762-B27BFA748B9C", PrettyName("Triggers")) = BIT(1),
	GITEM(StaticBodies, "6D41E880-6B7A-4BCA-974E-2C9129FE9251", PrettyName("Static Bodies")) = BIT(2),
	GITEM(KinematicBodies, "15EAE77F-E0E0-4767-9D82-88701F59E105", PrettyName("Kinematic Bodies")) = BIT(3),
	GITEM(DynamicBodies, "3E2D1C46-70CC-456C-A052-8F2C6C04712D", PrettyName("Dynamic Bodies")) = BIT(4)
};

GSTRUCT(PhysicsVisualizationSettings, "649AEBDD-0979-460B-87AC-38E68E15D40E", Serializable, PrettyName("Physics Visualization"))
{
	GFIELD("73F91948-D082-4289-BE43-315E616AF647", Serializable, PrettyName("Draw"))
	EnumFlag<PhysicsVisualizationFlag> flags = PhysicsVisualizationFlag::Colliders
										     | PhysicsVisualizationFlag::Triggers
										     | PhysicsVisualizationFlag::StaticBodies
										     | PhysicsVisualizationFlag::KinematicBodies
										     | PhysicsVisualizationFlag::DynamicBodies;

	GFIELD("A11B90BF-25A7-4946-99BD-8C32E2681F0A", Serializable, PrettyName("Depth Test"))
	bool depthTest = false;
};

} // namespace Gleam
