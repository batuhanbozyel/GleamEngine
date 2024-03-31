#include "gpch.h"
#include "Database.h"

using namespace Gleam::Reflection;

const ClassDescription& Database::GetClass(size_t hash)
{
	auto it = mClasses.find(hash);
	if (it != mClasses.end())
	{
		return it->second;
	}
	GLEAM_ASSERT(false, "Invalid class hash!");
	
	auto type = refl::reflect<Dummy>();
	return mClasses.insert(mClasses.end(), { hash, CreateClassDescription(type) })->second;
}
