#include "gpch.h"

#ifdef USE_DIRECTX_RENDERER
#include "Renderer/RayTracingScene.h"
#include "DirectXDevice.h"
#include "DirectXUtils.h"

#include "Renderer/Mesh.h"
#include "Renderer/Material/Material.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace Gleam;

void RayTracingScene::BuildAccelerationStructure(const CommandBuffer* cmd, const RenderSceneProxy* sceneProxy)
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mAllocator, mTLAS);
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

	Buffer instanceDescStagingBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "InstanceDescs Staging Buffer", .memoryType = MemoryType::CPU, .size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount });
	Buffer instanceDescsBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "InstanceDescs Buffer", .memoryType = MemoryType::GPU, .size = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount });

	uint32_t currentInstance = 0;
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(instanceDescStagingBuffer.GetContents());
	ID3D12GraphicsCommandList7* commandList = static_cast<ID3D12GraphicsCommandList7*>(cmd->GetHandle());
	sceneProxy->ForEach([&](const MeshBatch& batch)
	{
		for (uint32_t i = 0; i < batch.numInstances; ++i)
		{
			auto mesh = batch.meshes[i];
			if (not mesh->GetBLAS().IsValid())
			{
				const auto& submeshes = mesh->GetSubmeshes();
				auto indexBufferGpuAddress = static_cast<ID3D12Resource*>(mesh->GetIndexBuffer().GetHandle())->GetGPUVirtualAddress();
				auto positionBufferGpuAddress = static_cast<ID3D12Resource*>(mesh->GetPositionBuffer().GetHandle())->GetGPUVirtualAddress();

				TArray<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
				geometryDescs.reserve(submeshes.size());
				for (const auto& submesh : submeshes)
				{
					auto& desc = geometryDescs.emplace_back();
					desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
					desc.Triangles.IndexBuffer = indexBufferGpuAddress + submesh.firstIndex * sizeof(uint32_t);
					desc.Triangles.IndexCount = submesh.indexCount;
					desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
					desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					desc.Triangles.VertexCount = submesh.vertexCount;
					desc.Triangles.VertexBuffer.StartAddress = positionBufferGpuAddress + submesh.baseVertex * sizeof(float3);
					desc.Triangles.VertexBuffer.StrideInBytes = sizeof(float3);
					desc.Triangles.Transform3x4 = 0;
				}

				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
				bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
				bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
				bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
				bottomLevelInputs.NumDescs = (UINT)geometryDescs.size();
				bottomLevelInputs.pGeometryDescs = geometryDescs.data();

				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
				static_cast<ID3D12Device10*>(mDevice->GetHandle())->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);

				Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "BLAS Scratch Buffer", .memoryType = MemoryType::GPU, .size = prebuildInfo.ScratchDataSizeInBytes });
				BottomLevelAccelerationStructure blas = mDevice->CreateBLAS(mAllocator, BLASDescriptor{ .name = "BLAS", .size = prebuildInfo.ResultDataMaxSizeInBytes });

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
					bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_ALL;
					bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
					bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_COMMON;
					bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
					bufferBarrier[1].Offset = 0;
					bufferBarrier[1].Size = UINT64_MAX;

					D3D12_BARRIER_GROUP barrierGroup = {};
					barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
					barrierGroup.NumBarriers = _countof(bufferBarrier);
					barrierGroup.pBufferBarriers = bufferBarrier;
					commandList->Barrier(1, &barrierGroup);
				}

				bottomLevelBuildDesc.ScratchAccelerationStructureData = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle())->GetGPUVirtualAddress();
				bottomLevelBuildDesc.DestAccelerationStructureData = static_cast<ID3D12Resource*>(blas.GetHandle())->GetGPUVirtualAddress();
				commandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

				{
					D3D12_BUFFER_BARRIER bufferBarrier = {};
					bufferBarrier.SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
					bufferBarrier.SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
					bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
					bufferBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
					bufferBarrier.pResource = static_cast<ID3D12Resource*>(blas.GetHandle());
					bufferBarrier.Offset = 0;
					bufferBarrier.Size = UINT64_MAX;

					D3D12_BARRIER_GROUP barrierGroup = {};
					barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
					barrierGroup.NumBarriers = 1;
					barrierGroup.pBufferBarriers = &bufferBarrier;
					commandList->Barrier(1, &barrierGroup);
				}

				mesh->mBLAS = blas;
				mDevice->Dispose(mAllocator, scratchBuffer);
			}

			const auto& materialDesc = batch.material->GetDescriptor();
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

			const auto& instanceData = batch.instances[i];

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
			instanceDesc.InstanceID = i;
			instanceDesc.InstanceMask = 1;
			instanceDesc.InstanceContributionToHitGroupIndex = batch.material->GetPipelineHash(); // TODO: set to appropriate value
			instanceDesc.AccelerationStructure = static_cast<ID3D12Resource*>(mesh->GetBLAS().GetHandle())->GetGPUVirtualAddress();

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
	{
		D3D12_BUFFER_BARRIER bufferBarrier = {};
		bufferBarrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
		bufferBarrier.SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
		bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
		bufferBarrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
		bufferBarrier.pResource = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle());
		bufferBarrier.Offset = 0;
		bufferBarrier.Size = UINT64_MAX;

		D3D12_BARRIER_GROUP barrierGroup = {};
		barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		barrierGroup.NumBarriers = 1;
		barrierGroup.pBufferBarriers = &bufferBarrier;
		commandList->Barrier(1, &barrierGroup);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = static_cast<ID3D12Resource*>(instanceDescsBuffer.GetHandle())->GetGPUVirtualAddress();
	topLevelInputs.NumDescs = instanceCount;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	static_cast<ID3D12Device10*>(mDevice->GetHandle())->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &prebuildInfo);

	Buffer scratchBuffer = mDevice->CreateBuffer(mAllocator, BufferDescriptor{ .name = "TLAS Scratch Buffer", .memoryType = MemoryType::GPU, .size = prebuildInfo.ScratchDataSizeInBytes });
	TopLevelAccelerationStructure tlas = mDevice->CreateTLAS(mAllocator, TLASDescriptor{ .name = "TLAS", .size = prebuildInfo.ResultDataMaxSizeInBytes });

	{
		D3D12_BUFFER_BARRIER bufferBarrier[2] = {};
		bufferBarrier[0].SyncBefore = D3D12_BARRIER_SYNC_NONE;
		bufferBarrier[0].SyncAfter = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
		bufferBarrier[0].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		bufferBarrier[0].AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
		bufferBarrier[0].pResource = static_cast<ID3D12Resource*>(tlas.GetHandle());
		bufferBarrier[0].Offset = 0;
		bufferBarrier[0].Size = UINT64_MAX;

		bufferBarrier[1].SyncBefore = D3D12_BARRIER_SYNC_NONE;
		bufferBarrier[1].SyncAfter = D3D12_BARRIER_SYNC_ALL;
		bufferBarrier[1].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
		bufferBarrier[1].AccessAfter = D3D12_BARRIER_ACCESS_COMMON;
		bufferBarrier[1].pResource = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle());
		bufferBarrier[1].Offset = 0;
		bufferBarrier[1].Size = UINT64_MAX;

		D3D12_BARRIER_GROUP barrierGroup = {};
		barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		barrierGroup.NumBarriers = _countof(bufferBarrier);
		barrierGroup.pBufferBarriers = bufferBarrier;
		commandList->Barrier(1, &barrierGroup);
	}

	topLevelBuildDesc.ScratchAccelerationStructureData = static_cast<ID3D12Resource*>(scratchBuffer.GetHandle())->GetGPUVirtualAddress();
	topLevelBuildDesc.DestAccelerationStructureData = static_cast<ID3D12Resource*>(tlas.GetHandle())->GetGPUVirtualAddress();
	commandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

	{
		D3D12_BUFFER_BARRIER bufferBarrier = {};
		bufferBarrier.SyncBefore = D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
		bufferBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING;
		bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
		bufferBarrier.AccessAfter = D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
		bufferBarrier.pResource = static_cast<ID3D12Resource*>(tlas.GetHandle());
		bufferBarrier.Offset = 0;
		bufferBarrier.Size = UINT64_MAX;

		D3D12_BARRIER_GROUP barrierGroup = {};
		barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
		barrierGroup.NumBarriers = 1;
		barrierGroup.pBufferBarriers = &bufferBarrier;
		commandList->Barrier(1, &barrierGroup);
	}

	mDevice->Dispose(mAllocator, instanceDescStagingBuffer);
	mDevice->Dispose(mAllocator, instanceDescsBuffer);
	mDevice->Dispose(mAllocator, scratchBuffer);

	mTLAS = tlas;
}


#endif // USE_DIRECTX_RENDERER