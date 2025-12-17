#pragma once
#include "Math/Color.h"

namespace Gleam {

GSTRUCT(Sun, "C418E66E-3D0A-47A8-965A-6E39BB4799A8", EntityComponent, Serializable)
{
	GFIELD("1BB25490-F761-46E0-9CAD-A74DDFF9842B", Serializable)
	Color color = Color::white;

	GFIELD("EC8A7F1A-3430-428A-BB44-4BD07F297638", Serializable)
	float intensity = 1.0f;
};

} // namespace Gleam
