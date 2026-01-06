#include "gpch.h"
#include "ReflectionProbeRenderer.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

#include "World/World.h"

using namespace Gleam;

void ReflectionProbeRenderer::OnCreate(RenderContext& context)
{
	ComputePipelineStateDescriptor pipelineState;
	pipelineState.entryPoint = "skyAtmosphereRenderShader";
	mSkyRenderPipeline = context.device->CreateComputePipeline(pipelineState);
}

void ReflectionProbeRenderer::OnDestroy(RenderContext& context)
{

}

void ReflectionProbeRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
	const auto& sceneData = blackboard.Get<SceneRenderingData>();
	const auto& globalProbe = sceneData.world->GetEntityManager().GetSingletonComponent<ReflectionProbe>();

	struct CapturePassData
	{
		TextureHandle probe;
		TextureHandle transmittanceLut;
		TextureHandle multiScatterLut;
	};

	graph.AddComputePass<CapturePassData>("ReflectionProbe::Capture", [&](RenderGraphBuilder& builder, CapturePassData& passData)
	{
		TextureDescriptor textureDesc;
		textureDesc.name = "GlobalProbe";
		textureDesc.size = (float)globalProbe.size;
		textureDesc.usage |= TextureUsage_Storage;
		textureDesc.dimension = TextureDimension::TextureCube;
		textureDesc.format = TextureFormat::R16G16B16A16_SFloat;

		passData.probe = builder.CreateTexture(textureDesc);
		passData.transmittanceLut = builder.ReadTexture(sceneData.atmosphere.transmittanceLut);
		passData.multiScatterLut = builder.ReadTexture(sceneData.atmosphere.multiScatterLut);
	},
	[this, sceneData, globalProbe](const CommandBuffer* cmd, const CapturePassData& passData)
	{
		for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
		{
			CameraUniforms cubeFaceCamera = CreateCubeFaceCamera(sceneData.camera.uniforms.position, globalProbe.size, faceIndex);

			SkyAtmosphereRenderConstants constants = {};
			constants.targetTexture = passData.probe.GetTexture().GetResourceView(); // TODO: we need face slice here
			constants.depthTexture = InvalidResourceIndex; // No depth test needed
			constants.renderSun = 0; // Exclude sun for IBL

			cmd->BindComputePipeline(mSkyRenderPipeline);
			cmd->SetConstantBuffer(cubeFaceCamera, CAMERA_UNIFORMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.params, SKY_ATMOSPHERE_PARAMS_BINDING_SLOT);
			cmd->SetConstantBuffer(sceneData.atmosphere.uniforms, SKY_ATMOSPHERE_COMMON_UNIFORMS_BINDING_SLOT);
			cmd->SetPushConstant(constants);

			cmd->Dispatch(Math::DivideRoundingUp(globalProbe.size, 16u), Math::DivideRoundingUp(globalProbe.size, 16u), 1u);
		}
	});
}

CameraUniforms ReflectionProbeRenderer::CreateCubeFaceCamera(const float3& position, uint32_t resolution, uint32_t faceIndex)
{
	static const Float3 directions[6] = {
		Float3( 1,  0,  0), // +X
		Float3(-1,  0,  0), // -X
		Float3( 0,  1,  0), // +Y
		Float3( 0, -1,  0), // -Y
		Float3( 0,  0,  1), // +Z
		Float3( 0,  0, -1)  // -Z
	};
	static const Float3 ups[6] = {
		Float3(0, -1,  0), // +X
		Float3(0, -1,  0), // -X
		Float3(0,  0,  1), // +Y
		Float3(0,  0, -1), // -Y
		Float3(0, -1,  0), // +Z
		Float3(0, -1,  0)  // -Z
	};

	CameraUniforms camera = {};
	camera.resolution = (float)resolution;
	camera.viewMatrix = Float4x4::LookTo(position, directions[faceIndex], ups[faceIndex]);
	camera.projectionMatrix = Float4x4::Perspective(Math::PI / 2.0f, 1.0f, 1.0f, 10000.0f); // 90 degree FOV for cubemap
	camera.viewProjectionMatrix = camera.projectionMatrix * camera.viewMatrix;
	camera.invViewMatrix = Math::Inverse(camera.viewMatrix);
	camera.invProjectionMatrix = Math::Inverse(camera.projectionMatrix);
	camera.invViewProjectionMatrix = Math::Inverse(camera.viewProjectionMatrix);
	camera.position = position;
	return camera;
}