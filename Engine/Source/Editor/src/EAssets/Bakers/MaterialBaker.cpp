#include "MaterialBaker.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace GEditor;

MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset MaterialBaker::Bake(const Gleam::Filesystem::path& directory) const
{
	
}

const Gleam::TString& MaterialBaker::Filename() const
{
    return mDescriptor.name;
}
