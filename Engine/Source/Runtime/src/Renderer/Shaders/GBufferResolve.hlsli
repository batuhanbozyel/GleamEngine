#ifndef GBUFFER_RESOLVE_HLSL
#define GBUFFER_RESOLVE_HLSL

#define MOTION_VECTOR_PASS

#include "Common.hlsli"
#include "VisibilityBufferCommon.hlsli"

PUSH_CONSTANT(Gleam::GBufferResolveConstants, constants);

bool UnpackVisibilityShading(uint threadID, out Gleam::VisibilitySample visSample, out Gleam::BarycentricDeriv deriv)
{
    visSample = (Gleam::VisibilitySample) 0;
    deriv = (Gleam::BarycentricDeriv) 0;

    Gleam::MeshInstanceData instance;
    Gleam::VisibilityID visID;
    if (UnpackVisibilityPixel(constants.resolve, threadID, instance, visID, visSample.pixelCoords) == false)
    {
        return false;
    }
    
    Gleam::VertexAttributes attribs = LoadVertexAttributes(instance, visID.meshletID, visID.triangleID);
    float3 worldPos0 = mul(instance.transform, float4(attribs.positions[0], 1.0)).xyz;
    float3 worldPos1 = mul(instance.transform, float4(attribs.positions[1], 1.0)).xyz;
    float3 worldPos2 = mul(instance.transform, float4(attribs.positions[2], 1.0)).xyz;

    float4 clipPos0 = mul(camera.viewProjectionMatrix, float4(worldPos0, 1.0f));
    float4 clipPos1 = mul(camera.viewProjectionMatrix, float4(worldPos1, 1.0f));
    float4 clipPos2 = mul(camera.viewProjectionMatrix, float4(worldPos2, 1.0f));
    deriv = CalculateScreenSpaceBarycentrics(clipPos0, clipPos1, clipPos2, PixelSpaceToNDC(visSample.pixelCoords, camera.resolution), camera.resolution);
    
    float3 objectNormal = InterpolateBary(deriv, attribs.normals[0], attribs.normals[1], attribs.normals[2]);
    float3 objectTangent = InterpolateBary(deriv, attribs.tangents[0].xyz, attribs.tangents[1].xyz, attribs.tangents[2].xyz);
    float3 normal = normalize(mul(instance.transform, float4(objectNormal, 0.0f)).xyz);
    float3 tangent = normalize(mul(instance.transform, float4(objectTangent, 0.0f)).xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * attribs.tangents[0].w;
    
    visSample.vertex.position = float4(float2(visSample.pixelCoords) + 0.5f, 0.0f, 1.0f);
    visSample.vertex.worldPosition = InterpolateBary(deriv, worldPos0, worldPos1, worldPos2);
    visSample.vertex.normal = normal;
    visSample.vertex.tangent = tangent;
    visSample.vertex.bitangent = bitangent;
    visSample.vertex.color = InterpolateBary(deriv, attribs.colors[0], attribs.colors[1], attribs.colors[2]);
    InterpolateUV(deriv, attribs.texCoords[0], attribs.texCoords[1], attribs.texCoords[2], visSample.vertex.uv, visSample.vertex.ddxUV, visSample.vertex.ddyUV);
   
    float3 prevWorldPos0 = mul(instance.previousTransform, float4(attribs.positions[0], 1.0f)).xyz;
    float3 prevWorldPos1 = mul(instance.previousTransform, float4(attribs.positions[1], 1.0f)).xyz;
    float3 prevWorldPos2 = mul(instance.previousTransform, float4(attribs.positions[2], 1.0f)).xyz;
    visSample.vertex.prevWorldPosition = InterpolateBary(deriv, prevWorldPos0, prevWorldPos1, prevWorldPos2);
    
    visSample.surface = SurfMain(visSample.vertex);
    visSample.surface.roughness = max(visSample.surface.roughness, 0.04);
    return true;
}

[shader("compute")]
[numthreads(VISIBILITY_RESOLVE_GROUP_SIZE, 1, 1)]
void main(uint dispatchThreadID : SV_DispatchThreadID)
{
    Gleam::VisibilitySample visSample;
    Gleam::BarycentricDeriv deriv;

    [branch]
    if (UnpackVisibilityShading(dispatchThreadID, visSample, deriv))
    {
        float3x3 TBN = transpose(float3x3(visSample.vertex.tangent, visSample.vertex.bitangent, visSample.vertex.normal));
        float3 shadingNormal = mul(TBN, visSample.surface.normal);
    
        float4 prevClip = mul(camera.prevViewProjectionMatrix, float4(visSample.vertex.prevWorldPosition, 1.0f));
        float2 prevNdc = prevClip.xy / prevClip.w;
        float2 prevViewport = (prevNdc * float2(0.5f, -0.5f) + 0.5f) * camera.resolution;
        float2 motionVector = prevViewport - visSample.vertex.position.xy;

        RWTexture2D<float2> motionVectorTarget = ResourceDescriptorHeap[constants.motionVectorTarget];
        RWTexture2D<float2> geometryNormalTarget = ResourceDescriptorHeap[constants.geometryNormalTarget];
        RWTexture2D<float2> shadingNormalTarget = ResourceDescriptorHeap[constants.shadingNormalTarget];
        RWTexture2D<float> roughnessTarget = ResourceDescriptorHeap[constants.roughnessTarget];
        RWTexture2D<float2> barycentricCoordsTarget = ResourceDescriptorHeap[constants.barycentricCoords];
        RWTexture2D<uint4> barycentricDerivsTarget = ResourceDescriptorHeap[constants.barycentricDerivatives];

        motionVectorTarget[visSample.pixelCoords] = motionVector;
        geometryNormalTarget[visSample.pixelCoords] = OctEncode(normalize(visSample.vertex.normal));
        shadingNormalTarget[visSample.pixelCoords] = OctEncode(normalize(shadingNormal));
        roughnessTarget[visSample.pixelCoords] = visSample.surface.roughness;
        barycentricCoordsTarget[visSample.pixelCoords] = deriv.lambda.xy;
        barycentricDerivsTarget[visSample.pixelCoords] = PackBarycentricDerivatives(deriv);
    }
}
#endif // GBUFFER_RESOLVE_HLSL
