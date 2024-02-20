#include "Common.hlsli"
#include "../ShaderTypes.h"

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 majorLineColor : ATTRIBUTE0;
    float4 minorLineColor : ATTRIBUTE1;
    float3 nearPoint : ATTRIBUTE2;
    float3 farPoint : ATTRIBUTE3;
};

ConstantBuffer<Gleam::CameraUniforms> CameraBuffer : register(b0);
PUSH_CONSTANT(GEditor::InfiniteGridUniforms, uniforms);

VertexOut infiniteGridVertexShader(uint vertex_id: SV_VertexID)
{
    static const float3 gridPlane[6] = {
        float3( 1, -1, 0), float3(-1,  1, 0), float3(-1, -1, 0),
        float3(-1,  1, 0), float3( 1, -1, 0), float3( 1,  1, 0)
    };
    
    float3 clipPos = gridPlane[vertex_id];

    VertexOut OUT;
    OUT.position = float4(clipPos, 1.0f);
    OUT.nearPoint = ClipSpaceToWorldSpace(float3(clipPos.xy, 0.0f), CameraBuffer.invViewProjectionMatrix);
    OUT.farPoint = ClipSpaceToWorldSpace(float3(clipPos.xy, 1.0f), CameraBuffer.invViewProjectionMatrix);
    OUT.majorLineColor = unpack_unorm4x8_to_float(uniforms.majorLineColor);
    OUT.minorLineColor = unpack_unorm4x8_to_float(uniforms.minorLineColor);
    return OUT;
}

struct FragmentOut
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

float PristineGrid(float2 uv, float lineWidth)
{
    lineWidth = saturate(lineWidth);
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

float ComputeDepth(float3 pos)
{
    float4 clipPos = mul(CameraBuffer.viewProjectionMatrix, float4(pos, 1.0));
    return clipPos.z / clipPos.w;
}

FragmentOut infiniteGridFragmentShader(VertexOut IN)
{
    float t = -IN.nearPoint.y / (IN.farPoint.y - IN.nearPoint.y);
    float3 gridPos = IN.nearPoint + t * (IN.farPoint - IN.nearPoint);

    float division = max(2.0f, float(uniforms.majorGridDivision));
    float4 majorGrid = IN.majorLineColor * PristineGrid(gridPos.xz / division, uniforms.majorLineWidth / division);
    float4 minorGrid = IN.minorLineColor * PristineGrid(gridPos.xz, uniforms.minorLineWidth);

    FragmentOut OUT;
    OUT.depth = ComputeDepth(gridPos);
    OUT.color = saturate(minorGrid * (1.0f - majorGrid.a) + majorGrid) * float(t > 0.0f);
    return OUT;
}