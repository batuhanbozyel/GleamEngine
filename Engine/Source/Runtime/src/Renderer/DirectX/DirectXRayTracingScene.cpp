#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/RayTracingScene.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material/Material.h"
#include "World/Systems/RenderSceneProxy.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

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
	
	if (instanceCount == 0)
	{
		return {};
	}

	ID3D12GraphicsCommandList7* commandList = static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle());
	PIXBeginEvent(commandList, PIX_COLOR(128, 128, 255), "RayTracingScene::BuildAccelerationStructure");
	{
		Buffer instanceDescStagingBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "InstanceDescs Staging Buffer", .memoryType = MemoryType::CPU, .size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount });
		Buffer instanceDescsBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "InstanceDescs Buffer", .memoryType = MemoryType::GPU, .size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount });

		uint32_t currentInstance = 0;
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instanceDescStagingBuffer.GetContents());
		sceneProxy->ForEach([&](const MeshBatch& batch)
		{
			for (uint32_t i = 0; i < batch.numInstances; ++i)
			{
				const auto& instance = globalMeshes[batch.instanceOffset + i];
				const auto& submesh = instance.mesh->GetSubmesh(instance.submeshIndex);
				if (not instance.mesh->GetBLAS(instance.submeshIndex).IsValid())
				{
					PIXBeginEvent(commandList, PIX_COLOR(128, 0, 128), "BLAS");

					auto indexBufferGpuAddress = static_cast<ID3D12Resource*>(instance.mesh->GetIndexBuffer().GetHandle())->GetGPUVirtualAddress();
					auto positionBufferGpuAddress = static_cast<ID3D12Resource*>(instance.mesh->GetPositionBuffer().GetHandle())->GetGPUVirtualAddress();

					D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
					geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
					geometryDesc.Triangles.IndexBuffer = indexBufferGpuAddress + submesh.firstIndex * sizeof(uint32_t);
					geometryDesc.Triangles.IndexCount = submesh.indexCount;
					geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
					geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					geometryDesc.Triangles.VertexCount = submesh.vertexCount;
					geometryDesc.Triangles.VertexBuffer.StartAddress = positionBufferGpuAddress + submesh.baseVertex * sizeof(float3);
					geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(float3);
					geometryDesc.Triangles.Transform3x4 = 0;

					D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
					bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
					bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
					bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
					bottomLevelInputs.NumDescs = 1;
					bottomLevelInputs.pGeometryDescs = &geometryDesc;

					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
					static_cast<ID3D12Device10*>(mDevice->GetHandle())->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);

					static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>(); // Use persistent allocator for BLAS
					Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = instance.mesh->GetName() + ": BLAS Scratch Buffer", .memoryType = MemoryType::GPU, .size = prebuildInfo.ScratchDataSizeInBytes });
					BottomLevelAccelerationStructure blas = mDevice->CreateBLAS(BLASDescriptor{ .name = instance.mesh->GetName() + ": BLAS", .size = prebuildInfo.ResultDataMaxSizeInBytes });

					{
						D3D12_BUFFER_BARRIER bufferBarrier[2] = {};
						bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_NONE;
						bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
						bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
						bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
						bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(blas.GetHandle());
						bufferBarrier[0].Offset = 0;
						bufferBarrier[0].Size = UINT64_MAX;

						bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_NONE;
						bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
						bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
						bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
						bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
						bufferBarrier[1].Offset = 0;
						bufferBarrier[1].Size = UINT64_MAX;

						D3D12_BARRIER_GROUP barrierGroup = {};
						barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
						barrierGroup.NumBarriers = _countof(bufferBarrier);
						barrierGroup.pBufferBarriers = bufferBarrier;
						commandList->Barrier(1, &barrierGroup);
					}

					D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
					bottomLevelBuildDesc.Inputs = bottomLevelInputs;
					bottomLevelBuildDesc.ScratchAccelerationStructureData = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle())->GetGPUVirtualAddress();
					bottomLevelBuildDesc.DestAccelerationStructureData = static_cast<ID3D12Resource*>(blas.GetHandle())->GetGPUVirtualAddress();
					commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

					{
						D3D12_BUFFER_BARRIER bufferBarrier[2] = {};
						bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
						bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
						bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
						bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
						bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(blas.GetHandle());
						bufferBarrier[0].Offset = 0;
						bufferBarrier[0].Size = UINT64_MAX;

						bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
						bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_NONE;
						bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
						bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
						bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
						bufferBarrier[1].Offset = 0;
						bufferBarrier[1].Size = UINT64_MAX;

						D3D12_BARRIER_GROUP barrierGroup = {};
						barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
						barrierGroup.NumBarriers = _countof(bufferBarrier);
						barrierGroup.pBufferBarriers = bufferBarrier;
						commandList->Barrier(1, &barrierGroup);
					}

					mDevice->Dispose(mAllocator, scratchBuffer);
					instance.mesh->mBLASes[instance.submeshIndex] = blas;

					PIXEndEvent(commandList);
				}

				const auto& materialDesc = batch.material->GetDescriptor();
				const auto& instanceData = globalInstances[batch.instanceOffset + i];
				auto materialHash = batch.material->GetSurfaceShaderHash();

				UINT instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				if (materialDesc.cullingMode == CullMode::Off)
				{
					instanceFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
				}
				else if (materialDesc.cullingMode == CullMode::Front)
				{
					instanceFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
				}

				if (materialDesc.blendState.enabled)
				{
					instanceFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
				}
				else
				{
					instanceFlags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
				}

				D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[currentInstance];
				instanceDesc.Transform[0][0] = instanceData.transform[0][0];
				instanceDesc.Transform[0][1] = instanceData.transform[1][0];
				instanceDesc.Transform[0][2] = instanceData.transform[2][0];
				instanceDesc.Transform[0][3] = instanceData.transform[3][0];
				instanceDesc.Transform[1][0] = instanceData.transform[0][1];
				instanceDesc.Transform[1][1] = instanceData.transform[1][1];
				instanceDesc.Transform[1][2] = instanceData.transform[2][1];
				instanceDesc.Transform[1][3] = instanceData.transform[3][1];
				instanceDesc.Transform[2][0] = instanceData.transform[0][2];
				instanceDesc.Transform[2][1] = instanceData.transform[1][2];
				instanceDesc.Transform[2][2] = instanceData.transform[2][2];
				instanceDesc.Transform[2][3] = instanceData.transform[3][2];
				instanceDesc.Flags = instanceFlags;
				instanceDesc.InstanceID = batch.instanceOffset + i;
				instanceDesc.InstanceMask = 1;
				instanceDesc.InstanceContributionToHitGroupIndex = mHitGroupRegistry.GetIndex(materialHash);
				instanceDesc.AccelerationStructure = static_cast<ID3D12Resource*>(instance.mesh->GetBLAS(instance.submeshIndex).GetHandle())->GetGPUVirtualAddress();

				++currentInstance;
			}
		});

		{
			D3D12_BUFFER_BARRIER bufferBarrier[2] = {};
			bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_COPY;
			bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_COPY_SOURCE;
			bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(instanceDescStagingBuffer.GetHandle());
			bufferBarrier[0].Offset = 0;
			bufferBarrier[0].Size = UINT64_MAX;

			bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_COPY;
			bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST;
			bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle());
			bufferBarrier[1].Offset = 0;
			bufferBarrier[1].Size = UINT64_MAX;

			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
			barrierGroup.NumBarriers = _countof(bufferBarrier);
			barrierGroup.pBufferBarriers = bufferBarrier;
			commandList->Barrier(1, &barrierGroup);
		}
		cmd->CopyBuffer(instanceDescStagingBuffer, instanceDescsBuffer);

		PIXBeginEvent(commandList, PIX_COLOR(0, 128, 128), "TLAS");

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
		topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.InstanceDescs = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle())->GetGPUVirtualAddress();
		topLevelInputs.NumDescs = instanceCount;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
		static_cast<ID3D12Device10*>(mDevice->GetHandle())->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &prebuildInfo);

		Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "TLAS Scratch Buffer", .memoryType = MemoryType::GPU, .size = prebuildInfo.ScratchDataSizeInBytes });
		TopLevelAccelerationStructure tlas = mDevice->CreateTLAS(TLASDescriptor{ .name = "TLAS", .size = prebuildInfo.ResultDataMaxSizeInBytes, .instanceCount = instanceCount });
		{
			D3D12_BUFFER_BARRIER bufferBarrier[4] = {};
			bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
			bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(tlas.GetHandle());
			bufferBarrier[0].Offset = 0;
			bufferBarrier[0].Size = UINT64_MAX;

			bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
			bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
			bufferBarrier[1].Offset = 0;
			bufferBarrier[1].Size = UINT64_MAX;
			
			bufferBarrier[2].SyncBefore = D3D12_BARRIER_SYNC_COPY;
			bufferBarrier[2].SyncAfter = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[2].AccessBefore = D3D12_BARRIER_ACCESS_COPY_SOURCE;
			bufferBarrier[2].AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[2].pResource = static_cast<ID3D12Resource*>(instanceDescStagingBuffer.GetHandle());
			bufferBarrier[2].Offset = 0;
			bufferBarrier[2].Size = UINT64_MAX;

			bufferBarrier[3].SyncBefore = D3D12_BARRIER_SYNC_COPY;
			bufferBarrier[3].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[3].AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
			bufferBarrier[3].AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
			bufferBarrier[3].pResource = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle());
			bufferBarrier[3].Offset = 0;
			bufferBarrier[3].Size = UINT64_MAX;

			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
			barrierGroup.NumBarriers = _countof(bufferBarrier);
			barrierGroup.pBufferBarriers = bufferBarrier;
			commandList->Barrier(1, &barrierGroup);
		}
		mDevice->Dispose(mAllocator, instanceDescStagingBuffer);
		
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.ScratchAccelerationStructureData = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle())->GetGPUVirtualAddress();
		topLevelBuildDesc.DestAccelerationStructureData = static_cast<ID3D12Resource*>(tlas.GetHandle())->GetGPUVirtualAddress();
		commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

		{
			D3D12_BUFFER_BARRIER bufferBarrier[3] = {};
			bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING;
			bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
			bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
			bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(tlas.GetHandle());
			bufferBarrier[0].Offset = 0;
			bufferBarrier[0].Size = UINT64_MAX;

			bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
			bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
			bufferBarrier[1].Offset = 0;
			bufferBarrier[1].Size = UINT64_MAX;
			
			bufferBarrier[2].SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
			bufferBarrier[2].SyncAfter = D3D12_BARRIER_SYNC_NONE;
			bufferBarrier[2].AccessBefore = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
			bufferBarrier[2].AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
			bufferBarrier[2].pResource = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle());
			bufferBarrier[2].Offset = 0;
			bufferBarrier[2].Size = UINT64_MAX;

			D3D12_BARRIER_GROUP barrierGroup = {};
			barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
			barrierGroup.NumBarriers = _countof(bufferBarrier);
			barrierGroup.pBufferBarriers = bufferBarrier;
			commandList->Barrier(1, &barrierGroup);
		}
		mDevice->Dispose(mAllocator, scratchBuffer);
		mDevice->Dispose(mAllocator, instanceDescsBuffer);

		mTLAS = tlas;
		PIXEndEvent(commandList);
	}
	PIXEndEvent(commandList);

	return mTLAS.GetResourceView();
}
#endif // USE_DIRECTX_RENDERER
