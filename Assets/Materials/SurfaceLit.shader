float4 GetBaseColor(SurfaceInput IN)
{
    float4 baseColor = Material.BaseColor * IN.color;
    if (Material.BaseColorTexture.IsValid())
    {
        baseColor *= Material.BaseColorTexture.Sample(Sampler_Trilinear_Repeat, IN.uv);
    }
    return baseColor;
}

float3 GetWorldNormal(SurfaceInput IN)
{
    if (Material.NormalTexture.IsValid())
    {
        return 2.0 * Material.NormalTexture.Sample(Sampler_Trilinear_Repeat, IN.uv).rgb - 1.0;
    }
    return float3(0.0, 0.0, 1.0);
}

float4 GetEmission(SurfaceInput IN)
{
    float4 emission = Material.Emission;
    if (Material.EmissiveTexture.IsValid())
    {
        emission *= Material.EmissiveTexture.Sample(Sampler_Trilinear_Repeat, IN.uv);
    }
    return emission;
}

float3 GetMetallicRoughnessAo(SurfaceInput IN)
{
    float3 metallicRoughnessAo = float3(Material.Metallic, Material.Roughness, 1.0);
    if (Material.MetallicRoughnessTexture.IsValid())
    {
        metallicRoughnessAo *= Material.MetallicRoughnessTexture.Sample(Sampler_Trilinear_Repeat, IN.uv).rgb;
    }
    return metallicRoughnessAo;
}

void surf(SurfaceInput IN, inout SurfaceOutput OUT)
{
    float3 metallicRoughnessAo = GetMetallicRoughnessAo(IN);
    OUT.albedo = GetBaseColor(IN);
    OUT.emission = GetEmission(IN);
    OUT.normal = GetWorldNormal(IN);
    OUT.metallic = metallicRoughnessAo.r;
    OUT.roughness = metallicRoughnessAo.g;
    OUT.occlusion = metallicRoughnessAo.b;
}