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

PUSH_CONSTANT(GEditor::InfiniteGridUniforms, uniforms);

VertexOut infiniteGridVertexShader(uint vertex_id: SV_VertexID)
{
    Gleam::CameraUniforms CameraBuffer = uniforms.cameraBuffer.Load<Gleam::CameraUniforms>();

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

float PristineGrid(float2 uv, float lineWidth, float2 axisLines)
{
    lineWidth = saturate(lineWidth);
    float4 uvDDXY = float4(ddx(uv), ddy(uv));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    bool invertLine = lineWidth > 0.5;
    float2 targetWidth = invertLine ? 1.0 - lineWidth : lineWidth;
    float2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
    float2 lineAA = max(uvDeriv, 0.000001) * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = invertLine ? gridUV : 1.0 - gridUV;
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = saturate(grid2 - axisLines); // hack
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = invertLine ? 1.0 - grid2 : grid2;
    return lerp(grid2.x, 1.0, grid2.y);
}

float ComputeDepth(float3 pos, float4x4 viewProjectionMatrix)
{
    float4 clipPos = mul(viewProjectionMatrix, float4(pos, 1.0));
    return clipPos.z / clipPos.w;
}

FragmentOut infiniteGridFragmentShader(VertexOut IN)
{
    Gleam::CameraUniforms CameraBuffer = uniforms.cameraBuffer.Load<Gleam::CameraUniforms>();

    static const float4 AXIS_X_COLOR = float4(1.0f, 0.0f, 0.0f, 1.0f);
    static const float4 AXIS_Z_COLOR = float4(0.0f, 0.0f, 1.0f, 1.0f);
    
    float t = -IN.nearPoint.y / (IN.farPoint.y - IN.nearPoint.y);
    float3 gridPos = IN.nearPoint + t * (IN.farPoint - IN.nearPoint);
    
    float division = max(2.0f, float(uniforms.majorGridDivision));
    float majorLineWidth = saturate(uniforms.majorLineWidth / division);
    float minorLineWidth = saturate(uniforms.minorLineWidth);
    
    float2 uv = gridPos.xz;
    float2 majorUV = uv / division;
    
    // Axis lines
    float4 uvDDXY = float4(ddx(majorUV), ddy(majorUV));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    float2 axisDrawWidth = max(majorLineWidth, uvDeriv);
    float2 axisLineAA = uvDeriv * 1.5;
    float2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(majorUV * 2.0));
    axisLines2 *= saturate(majorLineWidth / axisDrawWidth);
    float4 axisLines = lerp(AXIS_X_COLOR * axisLines2.y, AXIS_Z_COLOR, axisLines2.x);
    
    float4 majorGrid = IN.majorLineColor * PristineGrid(majorUV, majorLineWidth, axisLines2);
    float4 minorGrid = IN.minorLineColor * PristineGrid(uv, minorLineWidth, axisLines2);
    float4 grid = saturate(minorGrid * (1.0f - majorGrid.a) + majorGrid);

    FragmentOut OUT;
    OUT.depth = ComputeDepth(gridPos, CameraBuffer.viewProjectionMatrix);
    OUT.color = saturate(grid * (1.0f - axisLines.a) + axisLines) * float(t > 0.0f);
    return OUT;
}
