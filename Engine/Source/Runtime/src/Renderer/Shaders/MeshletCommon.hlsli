#ifndef MESHLET_COMMON_HLSLI
#define MESHLET_COMMON_HLSLI

struct MeshletPayload
{
    uint meshletIDs[MESH_AMPLIFICATION_THREADS];
};

groupshared float4 gFrustumPlanes[6];
groupshared MeshletPayload gPayload;

float4 NormalizePlane(float4 plane)
{
    return plane / length(plane.xyz);
}

void BuildFrustumPlanes(float4x4 vp)
{
    gFrustumPlanes[0] = NormalizePlane(vp[3] + vp[0]);
    gFrustumPlanes[1] = NormalizePlane(vp[3] - vp[0]);
    gFrustumPlanes[2] = NormalizePlane(vp[3] + vp[1]);
    gFrustumPlanes[3] = NormalizePlane(vp[3] - vp[1]);
    gFrustumPlanes[4] = NormalizePlane(vp[2]);
    gFrustumPlanes[5] = NormalizePlane(vp[3] - vp[2]);
}

bool FrustumCullMeshlet(float3 worldCenter, float worldRadius)
{
    return dot(gFrustumPlanes[0], float4(worldCenter, 1.0f)) >= -worldRadius
        && dot(gFrustumPlanes[1], float4(worldCenter, 1.0f)) >= -worldRadius
        && dot(gFrustumPlanes[2], float4(worldCenter, 1.0f)) >= -worldRadius
        && dot(gFrustumPlanes[3], float4(worldCenter, 1.0f)) >= -worldRadius
        && dot(gFrustumPlanes[4], float4(worldCenter, 1.0f)) >= -worldRadius
        && dot(gFrustumPlanes[5], float4(worldCenter, 1.0f)) >= -worldRadius;
}

bool BackfaceCullMeshlet(float3 worldConeApex, float3 worldConeAxis, float coneCutoff, float3 cameraPosition)
{
    return dot(normalize(worldConeApex - cameraPosition), worldConeAxis) >= coneCutoff;
}

#endif // MESHLET_COMMON_HLSLI
