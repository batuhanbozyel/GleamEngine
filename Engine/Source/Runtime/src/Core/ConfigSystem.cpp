#include "gpch.h"
#include "ConfigSystem.h"
#include "Engine.h"
#include "Globals.h"

#include "Serialization/JSONSerializer.h"
#include "Serialization/JSONInternal.h"

using namespace Gleam;

void ConfigSystem::Initialize(Engine* engine)
{
	mSerializer = engine->GetSubsystem<JSONSerializer>();
	
	auto path = ConfigFilePath();
	if (Filesystem::Exists(path))
	{
		auto file = Filesystem::OpenRead(path, FileType::Text);
		rapidjson::IStreamWrapper ss(file->GetStream());
		rapidjson::Document document(rapidjson::kObjectType);
		document.ParseStream(ss);
		
		for (auto it = document.MemberBegin(); it != document.MemberEnd(); ++it)
		{
			const auto& element = it->value;
			const auto typeGuid = Guid(element["TypeGuid"].GetString());
			const auto classDesc = Reflection::IDatabase::GetInstance()->GetClass(typeGuid);
			if (classDesc == nullptr)
			{
				continue;
			}

			auto& block = RegisterBlock(*classDesc);
			mSerializer->Deserialize(*classDesc, block.data, rapidjson::ConstNode(element));
			block.Notify();
		}
	}
}

void ConfigSystem::Shutdown(Engine* engine)
{
	for (auto [typeHash, block] : mBlocks)
	{
		delete block;
	}
	mBlocks.clear();
}

Path ConfigSystem::ConfigFilePath() const
{
	return Globals::ProjectDirectory / "Engine.config";
}

void ConfigSystem::FlushToDisk() const
{
	rapidjson::Document document(rapidjson::kObjectType);
	for (const auto& [typeHash, block] : mBlocks)
	{
		const auto description = Reflection::GetClass(typeHash);
		const auto name = description->ResolveQualifiedName();
		
		rapidjson::Value blockValue(rapidjson::kObjectType);
		rapidjson::Node blockNode(blockValue, document.GetAllocator());
		mSerializer->Serialize(block->data, *description, blockNode);
		document.AddMember(rapidjson::StringRef(name.data(), name.size()), blockValue, document.GetAllocator());
	}

	auto file = Filesystem::Create(ConfigFilePath(), FileType::Text);
	rapidjson::OStreamWrapper ss(file->GetStream());
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	document.Accept(writer);
}

void ConfigSystem::ForEachConfig(const std::function<void(const ConfigView&)>& visitor)
{
	for (auto& [typeHash, block] : mBlocks)
	{
		const auto classDescription = Reflection::GetClass(typeHash);
		ConfigView view{ *classDescription, block->data };
		visitor(view);
	}
}

void ConfigSystem::MarkModified(uint32_t typeHash)
{
	auto it = mBlocks.find(typeHash);
	if (it != mBlocks.end())
	{
		FlushToDisk();
		it->second->Notify();
	}
}

ConfigSystem::ConfigBlock& ConfigSystem::RegisterBlock(const Reflection::ClassDescription& classDesc)
{
	if (HasBlock(classDesc))
	{
		return *mBlocks.at(classDesc.TypeHash());
	}
	
	auto block = new ConfigBlock(classDesc);
	mBlocks[classDesc.TypeHash()] = block;
	return *block;
}

ConfigSystem::ConfigBlock& ConfigSystem::GetBlock(const Reflection::ClassDescription& classDesc) const
{
	GLEAM_ASSERT(HasBlock(classDesc), "Config type is not registered!");
	auto it = mBlocks.find(classDesc.TypeHash());
	return *it->second;
}

bool ConfigSystem::HasBlock(const Reflection::ClassDescription& classDesc) const
{
	return mBlocks.find(classDesc.TypeHash()) != mBlocks.end();
}
