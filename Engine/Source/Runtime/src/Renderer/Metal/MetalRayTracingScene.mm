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

void RayTracingScene::BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy)
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}

	uint32_t instanceCount = 0;
	sceneProxy->ForEach([&](const MeshBatch& batch)
	{
		instanceCount += batch.numInstances;
	});

	if (instanceCount == 0)
	{
		return;
	}

	Buffer instanceDescBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{
		.name = "Instance Descriptors",
		.memoryType = MemoryType::CPU,
		.size = sizeof(MTLIndirectAccelerationStructureInstanceDescriptor) * instanceCount
	});

	id<MTLDevice> device = mDevice->GetHandle();
	id<MTL4CommandBuffer> commandBuffer = cmd->GetHandle();
	id<MTL4ComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

	uint32_t currentInstance = 0;
	MTLIndirectAccelerationStructureInstanceDescriptor* instanceDescs = static_cast<MTLIndirectAccelerationStructureInstanceDescriptor*>(instanceDescBuffer.GetContents());
	sceneProxy->ForEach([&](const MeshBatch& batch)
	{
		for (uint32_t i = 0; i < batch.numInstances; ++i)
		{
			auto mesh = batch.meshes[i];
			if (not mesh->GetBLAS().IsValid())
			{
				const auto& submeshes = mesh->GetSubmeshes();

				NSMutableArray<MTL4AccelerationStructureTriangleGeometryDescriptor*>* geometryDescs = [NSMutableArray arrayWithCapacity:submeshes.size()];

				id<MTLBuffer> indexBuffer = mesh->GetIndexBuffer().GetHandle();
				id<MTLBuffer> positionBuffer = mesh->GetPositionBuffer().GetHandle();

				for (const auto& submesh : submeshes)
				{
					MTL4AccelerationStructureTriangleGeometryDescriptor* geomDesc = [MTL4AccelerationStructureTriangleGeometryDescriptor new];
					geomDesc.vertexBuffer = MTL4BufferRange([positionBuffer gpuAddress] + submesh.baseVertex * sizeof(float3), submesh.vertexCount * sizeof(float3));
					geomDesc.vertexStride = sizeof(float3);
					geomDesc.vertexFormat = MTLAttributeFormatFloat3;
					geomDesc.triangleCount = submesh.indexCount / 3;

					geomDesc.indexBuffer = MTL4BufferRange([indexBuffer gpuAddress] + submesh.firstIndex * sizeof(uint32_t), submesh.indexCount * sizeof(uint32_t));
					geomDesc.indexType = MTLIndexTypeUInt32;
					geomDesc.opaque = YES;

					[geometryDescs addObject:geomDesc];
				}

				MTL4PrimitiveAccelerationStructureDescriptor* blasDesc = [MTL4PrimitiveAccelerationStructureDescriptor new];
				blasDesc.geometryDescriptors = geometryDescs;

				MTLAccelerationStructureSizes sizes = [device accelerationStructureSizesWithDescriptor:blasDesc];
				BottomLevelAccelerationStructure blas = mDevice->CreateBLAS(BLASDescriptor{ .name = mesh->GetName() + ": BLAS", .size = sizes.accelerationStructureSize });

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

				mesh->mBLAS = blas;
				mDevice->Dispose(mAllocator, scratchBuffer);
			}

			const auto& materialDesc = batch.material->GetDescriptor();
			const auto& instanceData = batch.instances[i];

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
			desc.userID = i;
			desc.mask = 1;
			desc.intersectionFunctionTableOffset = mHitGroupRegistry.GetIndex(pipelineHash); // TODO: check if this is valid
			desc.accelerationStructureID = [mesh->GetBLAS().GetHandle() gpuResourceID];

			++currentInstance;
		}
	});

	MTL4InstanceAccelerationStructureDescriptor* tlasDesc = [MTL4InstanceAccelerationStructureDescriptor new];
	tlasDesc.instanceCount = instanceCount;
	tlasDesc.instanceDescriptorBuffer = MTL4BufferRange([instanceDescBuffer.GetHandle() gpuAddress], sizeof(MTLIndirectAccelerationStructureInstanceDescriptor) * instanceCount);
	tlasDesc.instanceDescriptorStride = sizeof(MTLIndirectAccelerationStructureInstanceDescriptor);
	tlasDesc.instanceDescriptorType = MTLAccelerationStructureInstanceDescriptorTypeIndirect;
	tlasDesc.instanceTransformationMatrixLayout = MTLMatrixLayoutRowMajor;
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

	mDevice->Dispose(mAllocator, instanceDescBuffer);
	mDevice->Dispose(mAllocator, scratchBuffer);

	mTLAS = tlas;
}

#endif // USE_METAL_RENDERER
