//
//  ReflectionUtils.h
//  Editor
//

#pragma once
#include "Container/String.h"
#include "Reflection/Reflection.h"

namespace GEditor::ReflectionUtils {

inline Gleam::TStringView ResolveDisplayName(const Gleam::Reflection::ClassDescription& classDesc)
{
	if (classDesc.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		return classDesc.GetAttribute<Gleam::Reflection::Attribute::PrettyName>()->name;
	}
	return classDesc.ResolveName();
}

} // namespace GEditor::ReflectionUtils
