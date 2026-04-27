//
//  Material.cpp
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#include "gpch.h"
#include "Material.h"
#include "MaterialInstance.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"

#include "Renderer/Texture2D.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/CopyCommandBuffer.h"

#include "Assets/AssetManager.h"

using namespace Gleam;

static size_t ComputeMaterialInstanceSize(const TArray<MaterialProperty>& properties)
{
	size_t size = 0;
	for (const auto& property : properties)
	{
		switch (property.type)
		{
			case MaterialPropertyType::Scalar:
				size += sizeof(float);
				break;
			case MaterialPropertyType::Float2:
				size += sizeof(Float2);
				break;
			case MaterialPropertyType::Float3:
				size += sizeof(Float3);
				break;
			case MaterialPropertyType::Float4:
				size += sizeof(Float4);
				break;
			case MaterialPropertyType::Texture2D:
				size += sizeof(Texture2DResourceView<float4>);
				break;
		}
	}
	return size;
}

Material::Material(const MaterialDescriptor& descriptor)
    : IMaterial(descriptor.name, descriptor.properties)
	, mDescriptor(descriptor)
	, mPipelineStateHash((uint32_t)eastl::hash<MaterialDescriptor>()(descriptor))
	, mSurfaceShaderHash((uint32_t)eastl::hash<TString>()(descriptor.surfaceShader))
	, mInstanceDescriptorHeap(MaxMaterialInstances)
	, mInstanceSize(ComputeMaterialInstanceSize(descriptor.properties))
{
    static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();

	BufferDescriptor bufferDesc;
	bufferDesc.name = "Material: " + descriptor.name;
	bufferDesc.size = mInstanceSize * MaxMaterialInstances;
	mBuffer = device->CreateBuffer(renderSystem->GetAllocator(), bufferDesc);
}

Material::~Material()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto device = renderSystem->GetDevice();
	device->Dispose(renderSystem->GetAllocator(), mBuffer, BarrierStage::None);
}

ShaderResourceIndex Material::CreateInstance(const TArray<MaterialPropertyValue>& values)
{
	GLEAM_ASSERT(values.size() == mProperties.size(), "Material properties do not match with instance properties.");

	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();

	size_t offset = 0;
	TArray<uint8_t> instanceData(mInstanceSize);
	for (uint32_t i = 0; i < values.size(); ++i)
	{
		switch (mProperties[i].type)
		{
			case MaterialPropertyType::Scalar:
			{
				memcpy(OffsetPointer(instanceData.data(), offset), &values[i].scalar, sizeof(float));
				offset += sizeof(float);
				break;
			}
			case MaterialPropertyType::Float2:
			{
				memcpy(OffsetPointer(instanceData.data(), offset), &values[i].float2, sizeof(Float2));
				offset += sizeof(Float2);
				break;
			}
			case MaterialPropertyType::Float3:
			{
				memcpy(OffsetPointer(instanceData.data(), offset), &values[i].float3, sizeof(Float3));
				offset += sizeof(Float3);
				break;
			}
			case MaterialPropertyType::Float4:
			{
				memcpy(OffsetPointer(instanceData.data(), offset), &values[i].float4, sizeof(Float4));
				offset += sizeof(Float4);
				break;
			}
			case MaterialPropertyType::Texture2D:
			{
				const auto& asset = values[i].texture;
				if (asset.guid != Guid::InvalidGuid())
				{
					auto texture = assetManager->Load<Texture2D>(values[i].texture);
					Texture2DResourceView<float4> view = texture->GetResourceView();
					memcpy(OffsetPointer(instanceData.data(), offset), &view, sizeof(Texture2DResourceView<float4>));
				}
				else
				{
					Texture2DResourceView<float4> view = ShaderResourceIndex(InvalidResourceIndex);
					memcpy(OffsetPointer(instanceData.data(), offset), &view, sizeof(Texture2DResourceView<float4>));
				}
				offset += sizeof(Texture2DResourceView<float4>);
				break;
			}
			
		}
	}
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto cmd = renderSystem->GetCopyCommandBuffer();

	auto instance = mInstanceDescriptorHeap.Allocate();
	cmd->Commit(mBuffer, instanceData.data(), mInstanceSize, mInstanceSize * instance.data);
	return instance;
}

void Material::DestroyInstance(ShaderResourceIndex& instance)
{
	mInstanceDescriptorHeap.Release(instance);
	instance = InvalidResourceIndex;
}

const Buffer& Material::GetBuffer() const
{
    return mBuffer;
}

const MaterialDescriptor& Material::GetDescriptor() const
{
	return mDescriptor;
}

uint32_t Material::GetPipelineHash() const
{
	return mPipelineStateHash;
}

uint32_t Material::GetSurfaceShaderHash() const
{
	return mSurfaceShaderHash;
}

uint32_t Material::GetInstanceCount() const
{
	return mInstanceDescriptorHeap.GetSize();
}
