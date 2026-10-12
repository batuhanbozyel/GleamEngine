#include "Common.hlsli"
#include "ShaderTypes.h"

CONSTANT_BUFFER(Gleam::LineShaderResources, resources, 0);
CONSTANT_BUFFER(Gleam::CameraUniforms, camera, 1);

#define LINE_AA_MARGIN 1.0f

struct LineVertexOut
{
    float4 position : SV_POSITION;
    float4 color : ATTRIB0;
    noperspective float2 linePosition : ATTRIB1;
    nointerpolation float lineLength : ATTRIB2;
    nointerpolation float halfWidth : ATTRIB3;
};

static const float2 LINE_QUAD_CORNERS[6] =
{
    float2(0.0f, -1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f),
    float2(0.0f, -1.0f), float2(1.0f, 1.0f), float2(0.0f, 1.0f)
};

[shader("vertex")]
LineVertexOut lineVertexShader(uint vertex_id: SV_VertexID)
{
    ByteAddressBuffer lineBuffer = ResourceDescriptorHeap[resources.lineBuffer];
    Gleam::LineData lineData = lineBuffer.Load<Gleam::LineData>(resources.lineOffset + (vertex_id / 6) * sizeof(Gleam::LineData));
    float2 corner = LINE_QUAD_CORNERS[vertex_id % 6];
    float thickness = lineData.thickness * resources.thicknessScale;
    float halfWidth = max(thickness, 1.0f) * 0.5f;

    LineVertexOut OUT = (LineVertexOut)0;

    float3 viewStart = mul(camera.viewMatrix, float4(lineData.start, 1.0f)).xyz;
    float3 viewEnd = mul(camera.viewMatrix, float4(lineData.end, 1.0f)).xyz;
    if (viewStart.z < camera.nearPlane && viewEnd.z < camera.nearPlane)
    {
        OUT.position = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return OUT;
    }
    if (viewStart.z < camera.nearPlane)
    {
        viewStart = lerp(viewStart, viewEnd, (camera.nearPlane - viewStart.z) / (viewEnd.z - viewStart.z));
    }
    else if (viewEnd.z < camera.nearPlane)
    {
        viewEnd = lerp(viewEnd, viewStart, (camera.nearPlane - viewEnd.z) / (viewStart.z - viewEnd.z));
    }

    float4 clipStart = mul(camera.projectionMatrix, float4(viewStart, 1.0f));
    float4 clipEnd = mul(camera.projectionMatrix, float4(viewEnd, 1.0f));
    float2 screenStart = clipStart.xy / clipStart.w * 0.5f * camera.resolution;
    float2 screenEnd = clipEnd.xy / clipEnd.w * 0.5f * camera.resolution;

    float2 lineVector = screenEnd - screenStart;
    float lineLength = length(lineVector);
    float2 direction = lineLength > 1e-4f ? lineVector / lineLength : float2(1.0f, 0.0f);
    float2 normal = float2(-direction.y, direction.x);

    float extent = halfWidth + LINE_AA_MARGIN;
    bool isEnd = corner.x > 0.5f;
    float alongSign = isEnd ? 1.0f : -1.0f;

    float4 clip = isEnd ? clipEnd : clipStart;
    float2 offset = normal * corner.y * extent + direction * alongSign * extent;
    clip.xy += offset * 2.0f / camera.resolution * clip.w;

    OUT.position = clip;
    OUT.color = unpack_unorm4x8_to_float(lineData.color);
    OUT.color.a *= saturate(thickness);
    OUT.linePosition = float2(isEnd ? lineLength + extent : -extent, corner.y * extent);
    OUT.lineLength = lineLength;
    OUT.halfWidth = halfWidth;
    return OUT;
}

[shader("pixel")]
float4 lineFragmentShader(LineVertexOut IN) : SV_TARGET
{
    float capDistance = max(-IN.linePosition.x, IN.linePosition.x - IN.lineLength);
    float2 distance = float2(capDistance, abs(IN.linePosition.y));
    float2 coverage = saturate(IN.halfWidth + 0.5f - distance);
    return float4(IN.color.rgb, IN.color.a * coverage.x * coverage.y);
}
