float4 GetBaseColor(MeshVertexOut IN)
{
    float4 baseColor = Material.BaseColor * float4(IN.color, 1.0);
    if (Material.BaseColorTexture.IsValid())
    {
        baseColor *= Material.BaseColorTexture.Sample(Sampler_Trilinear_Repeat, IN.uv);
    }
    return baseColor;
}

float3 GetWorldNormal(MeshVertexOut IN)
{
    if (Material.NormalTexture.IsValid())
    {
        return normalize(2.0 * Material.NormalTexture.Sample(Sampler_Trilinear_Repeat, IN.uv).rgb - 1.0);
    }
    return float3(0.0, 0.0, 1.0);
}

float4 GetEmission(MeshVertexOut IN)
{
    float4 emission = Material.Emission;
    if (Material.EmissiveTexture.IsValid())
    {
        emission *= Material.EmissiveTexture.Sample(Sampler_Trilinear_Repeat, IN.uv);
    }
    return emission;
}

float2 GetMetallicRoughness(MeshVertexOut IN)
{
    float2 metallicRoughness = float2(Material.Metallic, Material.Roughness);
    if (Material.MetallicRoughnessTexture.IsValid())
    {
        metallicRoughness *= Material.MetallicRoughnessTexture.Sample(Sampler_Trilinear_Repeat, IN.uv).rg;
    }
    return metallicRoughness;
}

Gleam::SurfaceOutput surf(MeshVertexOut IN)
{
    float2 metallicRoughness = GetMetallicRoughness(IN);

    Gleam::SurfaceOutput OUT;
    OUT.albedo = GetBaseColor(IN);
    OUT.emission = GetEmission(IN);
    OUT.normal = GetWorldNormal(IN);
    OUT.metallic = metallicRoughness.r;
    OUT.roughness = metallicRoughness.g;
    return OUT;
}