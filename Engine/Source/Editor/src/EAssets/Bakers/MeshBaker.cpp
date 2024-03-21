#include "MeshBaker.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace GEditor;
using namespace rapidjson;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset MeshBaker::Bake(const Gleam::Filesystem::path& directory) const
{
    Gleam::Guid guid = Gleam::Guid::NewGuid();
    Gleam::Asset asset(guid);
    
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    
    writer.StartObject();
    
    writer.EndObject();
    return asset;
}

const Gleam::TString& MeshBaker::Filename() const
{
    return mDescriptor.name;
}
