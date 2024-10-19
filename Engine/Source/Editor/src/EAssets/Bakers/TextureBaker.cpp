#include "TextureBaker.h"

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::Texture2DDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

void TextureBaker::Bake(Gleam::FileStream& stream) const
{
	auto serializer = Gleam::JSONSerializer(stream);
	serializer.Serialize(mDescriptor);
}

Gleam::TString TextureBaker::Filename() const
{
	return mDescriptor.name;
}

Gleam::Guid TextureBaker::TypeGuid() const
{
    return Gleam::Reflection::GetClass<decltype(mDescriptor)>().Guid();
}
