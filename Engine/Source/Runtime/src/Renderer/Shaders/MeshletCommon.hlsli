#ifndef MESHLET_COMMON_HLSLI
#define MESHLET_COMMON_HLSLI

#include "ShaderTypes.h"

struct MeshletPayload
{
    uint meshletIDs[MESH_AMPLIFICATION_THREADS];
};

groupshared MeshletPayload gPayload;

bool FrustumCullMeshlet(Gleam::Frustum frustum, float3 worldCenter, float worldRadius)
{
    for (int i = 0; i < 6; ++i)
    {
        float distance = dot(frustum.planes[i], float4(worldCenter, 1.0f));
        if (distance < -worldRadius)
        {
            return true; // Cull the meshlet
        }
    }
    return false;
}

bool BackfaceCullMeshlet(float3 worldConeApex, float3 worldConeAxis, float coneCutoff, float3 cameraPosition, Gleam::CullMode cullMode)
{
    float cosAngle = dot(normalize(worldConeApex - cameraPosition), worldConeAxis);
    if (cullMode == Gleam::CullMode::Back)
    {
        return cosAngle >= coneCutoff;
    }
    else if (cullMode == Gleam::CullMode::Front)
    {
        return cosAngle <= -coneCutoff;
    }
    else
    {
        return false;
    }
}

bool MeshletIsVisible(Gleam::MeshInstanceData instanceData, Gleam::MeshletDescriptor meshlet, Gleam::CameraUniforms camera)
{
    float3 worldCenter = mul(instanceData.transform, float4(meshlet.center, 1.0f)).xyz;
    float3 worldConeApex = mul(instanceData.transform, float4(meshlet.coneApex, 1.0f)).xyz;
    float3 worldConeAxis = normalize(mul(instanceData.transform, float4(meshlet.coneAxis, 0.0f)).xyz);
    float scale = length(mul(instanceData.transform, float4(1.0f, 0.0f, 0.0f, 0.0f)).xyz);
    
    if (FrustumCullMeshlet(camera.frustum, worldCenter, meshlet.radius * scale))
    {
        return false;
    }
    if (BackfaceCullMeshlet(worldConeApex, worldConeAxis, meshlet.coneCutoff, camera.position, (Gleam::CullMode)instanceData.cullMode))
    {
        return false;
    }
    return true;
}

#endif // MESHLET_COMMON_HLSLI
