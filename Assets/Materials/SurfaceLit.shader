float4 GetBaseColor(Gleam::MeshVertexOut IN)
{
    float4 baseColor = Material.BaseColor * IN.color;
    if (Material.BaseColorTexture.IsValid())
    {
        baseColor *= Material.BaseColorTexture.SampleGrad(Sampler_Anisotropic_Repeat, IN.uv, IN.ddxUV, IN.ddyUV);
    }
    return baseColor;
}

float3 GetWorldNormal(Gleam::MeshVertexOut IN)
{
    if (Material.NormalTexture.IsValid())
    {
        return normalize(Material.NormalTexture.SampleGrad(Sampler_Anisotropic_Repeat, IN.uv, IN.ddxUV, IN.ddyUV).rgb * 2.0 - 1.0);
    }
    return float3(0.0, 0.0, 1.0);
}

float4 GetEmission(Gleam::MeshVertexOut IN)
{
    float4 emission = Material.Emission;
    if (Material.EmissiveTexture.IsValid())
    {
        emission *= Material.EmissiveTexture.SampleGrad(Sampler_Anisotropic_Repeat, IN.uv, IN.ddxUV, IN.ddyUV);
    }
    return emission;
}

float2 GetMetallicRoughness(Gleam::MeshVertexOut IN)
{
    float2 metallicRoughness = float2(Material.Metallic, Material.Roughness);
    if (Material.MetallicRoughnessTexture.IsValid())
    {
        metallicRoughness *= Material.MetallicRoughnessTexture.SampleGrad(Sampler_Anisotropic_Repeat, IN.uv, IN.ddxUV, IN.ddyUV).bg;
    }
    return metallicRoughness;
}

Gleam::SurfaceOutput SurfMain(Gleam::MeshVertexOut IN)
{
    float2 metallicRoughness = GetMetallicRoughness(IN);

    Gleam::SurfaceOutput OUT;
    OUT.albedo = GetBaseColor(IN);
    OUT.emission = GetEmission(IN);
    OUT.normal = GetWorldNormal(IN);
    OUT.metallic = metallicRoughness.r;
    OUT.roughness = metallicRoughness.g;
    OUT.alphaCutoff = Material.AlphaCutoff;
    return OUT;
}
