#include "MaterialBaker.h"

using namespace GEditor;

// MaterialBaker
MaterialBaker::MaterialBaker(const Gleam::MaterialDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void MaterialBaker::Bake(Gleam::FileStream& stream) const
{
	auto serializer = Gleam::JSONSerializer();
	serializer.Serialize(mDescriptor, stream);
}

Gleam::TString MaterialBaker::Filename() const
{
    return mDescriptor.name;
}

Gleam::Guid MaterialBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::MaterialDescriptor& MaterialBaker::GetDescriptor() const
{
	return mDescriptor;
}

// MaterialInstanceBaker
MaterialInstanceBaker::MaterialInstanceBaker(const Gleam::MaterialInstanceDescriptor& descriptor)
	: mDescriptor(descriptor)
{

}

void MaterialInstanceBaker::Bake(Gleam::FileStream& stream) const
{
	auto serializer = Gleam::JSONSerializer();
	serializer.Serialize(mDescriptor, stream);
}

Gleam::TString MaterialInstanceBaker::Filename() const
{
	return mDescriptor.name;
}

Gleam::Guid MaterialInstanceBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}

const Gleam::MaterialInstanceDescriptor& MaterialInstanceBaker::GetDescriptor() const
{
	return mDescriptor;
}