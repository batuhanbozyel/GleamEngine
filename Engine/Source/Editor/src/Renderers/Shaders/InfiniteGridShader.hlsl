#include "Common.hlsli"
#include "../ShaderTypes.h"

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

ConstantBuffer<Gleam::CameraUniforms> CameraBuffer : register(b0);
PUSH_CONSTANT(GEditor::InfiniteGridUniforms, uniforms);

VertexOut infiniteGridVertexShader(uint vertex_id: SV_VertexID)
{
    static const float3 gridPlane[6] = {
        float3(-1, -1, 0), float3(1, -1, 0), float3(1, 1, 0),
        float3(-1, -1, 0), float3(1,  1, 0), float3(-1, 1, 0)
    };
    
    float3 clipPos = gridPlane[vertex_id];
    float3 worldPos = ClipSpaceToWorldSpace(clipPos, CameraBuffer.invViewMatrix, CameraBuffer.invProjectionMatrix);
    float3 cameraCenteringOffset = floor(CameraBuffer.worldPosition);
    float3 cameraSnappedWorldPos = worldPos - cameraCenteringOffset;

    VertexOut OUT;
    OUT.position = float4(clipPos, 1.0f);
    OUT.uv = cameraSnappedWorldPos.xz;
    OUT.color = unpack_unorm4x8_to_float(uniforms.color);
    return OUT;
}

float PristineGrid(float2 uv)
{
    float lineWidth = saturate(uniforms.lineWidth);
    float4 uvDDXY = float4(ddx(uv), ddy(uv));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    bool2 invertLine = lineWidth > 0.5;
    float2 targetWidth = invertLine ? 1.0 - lineWidth : lineWidth;
    float2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
    float2 lineAA = max(uvDeriv, 0.000001) * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = invertLine ? gridUV : 1.0 - gridUV;
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = invertLine ? 1.0 - grid2 : grid2;
    return lerp(grid2.x, 1.0, grid2.y);
}

float4 infiniteGridFragmentShader(VertexOut IN) : SV_TARGET
{
    float grid = PristineGrid(IN.uv);
    return IN.color * grid;
}