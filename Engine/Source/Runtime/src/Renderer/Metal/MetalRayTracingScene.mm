#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RayTracingScene.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/Material.h"
#include "World/Systems/RenderSceneProxy.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include <metal_irconverter_runtime/metal_irconverter_runtime.h>
#include <metal_irconverter_runtime/ir_raytracing.h>

using namespace Gleam;

AccelerationStructureView RayTracingScene::BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy)
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}

	const auto globalInstances = sceneProxy->GetGlobalInstances();
	const auto globalMeshes = sceneProxy->GetGlobalMeshes();
	uint32_t instanceCount = (uint32_t)globalInstances.size();

	id<MTLDevice> device = mDevice->GetHandle();
	id<MTL4CommandBuffer> commandBuffer = cmd->GetHandle();
	id<MTL4ComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

	Buffer instanceDescBuffer;

	if (instanceCount > 0)
	{
		instanceDescBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{
			.name = "Instance Descriptors",
			.memoryType = MemoryType::CPU,
			.size = sizeof(MTLIndirectAccelerationStructureInstanceDescriptor) * instanceCount
		});

		uint32_t currentInstance = 0;
		MTLIndirectAccelerationStructureInstanceDescriptor* instanceDescs = static_cast<MTLIndirectAccelerationStructureInstanceDescriptor*>(instanceDescBuffer.GetContents());
		sceneProxy->ForEach([&](const MeshBatch& batch)
		{
			for (uint32_t i = 0; i < batch.numInstances; ++i)
			{
				const auto& instance = globalMeshes[batch.instanceOffset + i];
				const auto& submesh = instance.mesh->GetSubmesh(instance.submeshIndex);
				if (not instance.mesh->GetBLAS(instance.submeshIndex).IsValid())
				{
					id<MTLBuffer> indexBuffer = instance.mesh->GetIndexBuffer().GetHandle();
					id<MTLBuffer> positionBuffer = instance.mesh->GetPositionBuffer().GetHandle();

					MTL4AccelerationStructureTriangleGeometryDescriptor* geomDesc = [MTL4AccelerationStructureTriangleGeometryDescriptor new];
					geomDesc.vertexBuffer = MTL4BufferRange([positionBuffer gpuAddress] + submesh.baseVertex * sizeof(float3), submesh.vertexCount * sizeof(float3));
					geomDesc.vertexStride = sizeof(float3);
					geomDesc.vertexFormat = MTLAttributeFormatFloat3;
					geomDesc.triangleCount = submesh.indexCount / 3;
					geomDesc.indexBuffer = MTL4BufferRange([indexBuffer gpuAddress] + submesh.firstIndex * sizeof(uint32_t), submesh.indexCount * sizeof(uint32_t));
					geomDesc.indexType = MTLIndexTypeUInt32;
					geomDesc.opaque = YES;

					MTL4PrimitiveAccelerationStructureDescriptor* blasDesc = [MTL4PrimitiveAccelerationStructureDescriptor new];
					blasDesc.geometryDescriptors = [NSArray arrayWithObject:geomDesc];

					MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor:blasDesc];
					BottomLevelAccelerationStructure blas = mDevice->CreateBLAS(BLASDescriptor{ .name = instance.mesh->GetName() + ": BLAS", .size = sizes.accelerationStructureSize });

					Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{
						.name = "BLAS Scratch Buffer",
						.memoryType = MemoryType::GPU,
						.size = sizes.buildScratchBufferSize
					});
					MTL4BufferRange scratchRange([scratchBuffer.GetHandle() gpuAddress], sizes.buildScratchBufferSize);

					[static_cast<MetalDevice*>(mDevice)->GetResidencySet() commit];
					[encoder buildAccelerationStructure:blas.GetHandle()
											 descriptor:blasDesc
										  scratchBuffer:scratchRange];

					[encoder barrierAfterEncoderStages:MTLStageAccelerationStructure
									beforeEncoderStages:MTLStageAccelerationStructure
									  visibilityOptions:MTL4VisibilityOptionNone];

					instance.mesh->mBLASes[instance.submeshIndex] = blas;
					mDevice->Dispose(mAllocator, scratchBuffer);
				}

				const auto& materialDesc = batch.material->GetDescriptor();
				const auto& instanceData = globalInstances[batch.instanceOffset + i];
				auto materialHash = batch.material->GetSurfaceShaderHash();

				MTLAccelerationStructureInstanceOptions options = MTLAccelerationStructureInstanceOptionNone;
				if (materialDesc.cullingMode == CullMode::Off)
				{
					options |= MTLAccelerationStructureInstanceOptionDisableTriangleCulling;
				}
				else if (materialDesc.cullingMode == CullMode::Front)
				{
					options |= MTLAccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
				}

				if (materialDesc.blendState.enabled)
				{
					options |= MTLAccelerationStructureInstanceOptionNonOpaque;
				}
				else
				{
					options |= MTLAccelerationStructureInstanceOptionOpaque;
				}

				MTLIndirectAccelerationStructureInstanceDescriptor& desc = instanceDescs[currentInstance];
				desc.transformationMatrix.columns[0] = MTLPackedFloat3Make(
					instanceData.transform[0][0],
					instanceData.transform[0][1],
					instanceData.transform[0][2]);
				desc.transformationMatrix.columns[1] = MTLPackedFloat3Make(
					instanceData.transform[1][0],
					instanceData.transform[1][1],
					instanceData.transform[1][2]);
				desc.transformationMatrix.columns[2] = MTLPackedFloat3Make(
					instanceData.transform[2][0],
					instanceData.transform[2][1],
					instanceData.transform[2][2]);
				desc.transformationMatrix.columns[3] = MTLPackedFloat3Make(
					instanceData.transform[3][0],
					instanceData.transform[3][1],
					instanceData.transform[3][2]);

				desc.options = options;
				desc.userID = batch.instanceOffset + i;
				desc.mask = 1;
				desc.intersectionFunctionTableOffset = mHitGroupRegistry.GetIndex(materialHash) * (uint32_t)RayType::COUNT;
				desc.accelerationStructureID = [instance.mesh->GetBLAS(instance.submeshIndex).GetHandle() gpuResourceID];

				++currentInstance;
			}
		});
	}

	MTL4InstanceAccelerationStructureDescriptor* tlasDesc = [MTL4InstanceAccelerationStructureDescriptor new];
	tlasDesc.instanceCount = instanceCount;
	tlasDesc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeIndirect;
	tlasDesc.instanceTransformationMatrixLayout = MTLMatrixLayoutRowMajor;

	if (instanceCount > 0)
	{
		tlasDesc.instanceDescriptorBuffer = MTL4BufferRange([instanceDescBuffer.GetHandle() gpuAddress], sizeof(MTLIndirectAccelerationStructureInstanceDescriptor) * instanceCount);
		tlasDesc.instanceDescriptorStride = sizeof(MTLIndirectAccelerationStructureInstanceDescriptor);
	}

	MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor:tlasDesc];

	TopLevelAccelerationStructure tlas = mDevice->CreateTLAS(TLASDescriptor{
		.name = "TLAS",
		.size = sizes.accelerationStructureSize
	});

	Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{
		.name = "TLAS Scratch Buffer",
		.memoryType = MemoryType::GPU,
		.size = sizes.buildScratchBufferSize
	});
	MTL4BufferRange scratchRange([scratchBuffer.GetHandle() gpuAddress], sizes.buildScratchBufferSize);

	[static_cast<MetalDevice*>(mDevice)->GetResidencySet() commit];
	[encoder buildAccelerationStructure:tlas.GetHandle()
							 descriptor:tlasDesc
						  scratchBuffer:scratchRange];

	[encoder barrierAfterEncoderStages:MTLStageAccelerationStructure
					beforeEncoderStages:MTLStageDispatch
					  visibilityOptions:MTL4VisibilityOptionNone];

	[encoder endEncoding];

	if (instanceCount > 0)
	{
		mDevice->Dispose(mAllocator, instanceDescBuffer);
	}
	mDevice->Dispose(mAllocator, scratchBuffer);

	mTLAS = tlas;
	return mTLAS.GetResourceView();
}

#endif // USE_METAL_RENDERER