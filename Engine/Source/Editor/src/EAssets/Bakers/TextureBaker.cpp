#include "TextureBaker.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace GEditor;

TextureBaker::TextureBaker(const Gleam::TextureDescriptor& descriptor)
	: mDescriptor(descriptor)
{
	
}

Gleam::Asset TextureBaker::Bake(const Gleam::Filesystem::path& directory) const
{
	
}

const Gleam::TString& TextureBaker::Filename() const
{
    
}
