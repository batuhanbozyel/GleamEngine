#include "Common.hlsli"
#include "ShaderTypes.h"

PUSH_CONSTANT(Gleam::ImGuiResources, constants);

struct VertexOut
{
	float4 pos : SV_POSITION;
	float2 uv : ATTRIB0;
	float4 color : ATTRIB1;
};

#pragma vertex imguiVertexShader
#pragma fragment imguiFragmentShader

VertexOut imguiVertexShader(uint vertexID : SV_VERTEXID)
{
	ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[SRVIndex(constants.vertexBuffer)];
	const uint vertexOffset = constants.vertexOffset + (vertexID * 20);
	float2 position = asfloat(vertexBuffer.Load2(vertexOffset));

    VertexOut OUT;
	OUT.pos = mul(constants.projMatrix, float4(position, 0.0f, 1.0f));
	OUT.uv = asfloat(vertexBuffer.Load2(vertexOffset + 8));
	OUT.color = unpack_unorm4x8_to_float(asuint(vertexBuffer.Load(vertexOffset + 16)));
	return OUT;
}

float4 imguiFragmentShader(VertexOut IN) : SV_Target
{
	Texture2D<float4> texture = ResourceDescriptorHeap[SRVIndex(constants.texture)];
	return texture.SampleLevel(Sampler_Bilinear_Repeat, IN.uv, 0) * IN.color;
}
