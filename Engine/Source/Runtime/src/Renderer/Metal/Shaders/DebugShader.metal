#include "../../ShaderTypes.h"

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex VertexOut debugVertexShader(uint vertexID [[vertex_id]],
                                   constant Gleam::DebugVertex* VertexBuffer [[buffer(0)]],
                                   constant Gleam::CameraUniforms& cameraUniforms [[buffer(1)]])
{
    Gleam::DebugVertex vert = VertexBuffer[vertexID];

    VertexOut out;
    out.position = cameraUniforms.viewProjectionMatrix * float4(vert.position, 1.0);
    out.color = unpack_unorm4x8_to_float(vert.color);
    return out;
}

vertex VertexOut debugMeshVertexShader(uint vertexID [[vertex_id]],
                                       constant Gleam::Vector3* PositionBuffer [[buffer(0)]],
                                       constant Gleam::CameraUniforms& cameraUniforms [[buffer(1)]])
{
    VertexOut out;
    out.position = cameraUniforms.modelViewProjectionMatrix * float4(PositionBuffer[vertexID], 1.0);
    out.color = float4(1.0);
    return out;
}

fragment float4 debugFragmentShader(VertexOut in [[stage_in]],
                                    constant Gleam::DebugFragmentUniforms& fragmentUniforms [[buffer(0)]])
{
    return in.color * unpack_unorm4x8_to_float(fragmentUniforms.color);
}
