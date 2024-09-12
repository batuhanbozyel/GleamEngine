#include "MeshBaker.h"

using namespace GEditor;

MeshBaker::MeshBaker(const Gleam::MeshDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void MeshBaker::Bake(Gleam::FileStream& stream) const
{
	auto serializer = Gleam::JSONSerializer(stream);
	serializer.Serialize(mDescriptor);
}

Gleam::TString MeshBaker::Filename() const
{
    return mDescriptor.name;
}

Gleam::Guid MeshBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}
