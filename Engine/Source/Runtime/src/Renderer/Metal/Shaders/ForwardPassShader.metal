#include "../../ShaderTypes.h"

struct VertexOut
{
    float4 position [[position]];
    float3 normal;
    float2 texCoord;
};

vertex VertexOut forwardPassVertexShader(uint vertexID [[vertex_id]],
                                         constant Gleam::Vector3* PositionBuffer [[buffer(0)]],
                                         constant Gleam::InterleavedMeshVertex* InterleavedBuffer [[buffer(1)]],
                                         constant Gleam::CameraUniforms& cameraUniforms [[buffer(2)]])
{
    Gleam::InterleavedMeshVertex interleavedVert = InterleavedBuffer[vertexID];

    VertexOut out;
    out.position = cameraUniforms.modelViewProjectionMatrix * float4(PositionBuffer[vertexID], 1.0);
    out.normal = cameraUniforms.normalMatrix * float3(interleavedVert.normal);
    out.texCoord = interleavedVert.texCoord;
    return out;
}

fragment float4 forwardPassFragmentShader(VertexOut in [[stage_in]],
                                          constant Gleam::ForwardPassFragmentUniforms& fragmentUniforms [[buffer(0)]])
{
    return unpack_unorm4x8_to_float(fragmentUniforms.color);
}
