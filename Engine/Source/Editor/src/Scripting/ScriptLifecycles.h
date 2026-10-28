//
//  ScriptLifecycles.h
//  Editor
//

#pragma once
#include "Container/Array.h"

namespace GEditor {

class ComposedLifecycle;

class ScriptLifecycles final
{
public:

	void RegisterAll();

	void UnregisterAll();

	const Gleam::TArray<uint32_t>& GetTypes() const
	{
		return mTypes;
	}

private:

	Gleam::TArray<uint32_t> mTypes;

	Gleam::TArray<ComposedLifecycle*> mComposed;
};

} // namespace GEditor
