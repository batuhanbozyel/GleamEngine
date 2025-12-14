#include "SkyAtmosphereCommon.hlsli"

#pragma compute skyAtmosphereMultiScatterLUTShader

struct MultiScatteringResult
{
	float3 L; // Scattered light (luminance)
	float3 MultiScatAs1;
};

MultiScatteringResult IntegrateMultiScattering(
	in float3 WorldPos, in float3 WorldDir,
	in float3 SunDir, in float SampleCountIni, in float tMaxMax = 9000000.0f)
{
	MultiScatteringResult result = (MultiScatteringResult) 0;
	
	// Compute next intersection with atmosphere or ground 
	float3 earthO = float3(0.0f, 0.0f, 0.0f);
	float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, earthO, atmosphereParams.bottomRadius);
	float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, earthO, atmosphereParams.topRadius);
	float tMax = 0.0f;
	if (tBottom < 0.0f)
	{
		if (tTop < 0.0f)
		{
			tMax = 0.0f; // No intersection with earth nor atmosphere: stop right away  
			return result;
		}
		else
		{
			tMax = tTop;
		}
	}
	else
	{
		if (tTop > 0.0f)
		{
			tMax = min(tTop, tBottom);
		}
	}
	tMax = min(tMax, tMaxMax);

	// Sample count 
	float SampleCount = SampleCountIni;
	float dt = tMax / SampleCount;

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);

	// Ray march the atmosphere to integrate optical depth
	float3 throughput = 1.0;
	float t = 0.0f;
	float tPrev = 0.0;
	const float SampleSegmentT = 0.3f;
	for (float s = 0.0f; s < SampleCount; s += 1.0f)
	{
		//t = tMax * (s + SampleSegmentT) / SampleCount;
		// Exact difference, important for accuracy of multiple scattering
		float NewT = tMax * (s + SampleSegmentT) / SampleCount;
		dt = NewT - t;
		t = NewT;
		float3 P = WorldPos + t * WorldDir;

		MediumSampleRGB medium = SampleMediumRGB(P);
		const float3 SampleOpticalDepth = medium.extinction * dt;
		const float3 SampleTransmittance = exp(-SampleOpticalDepth);
		
		float pHeight = length(P);
		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;
		float3 PhaseTimesScattering = medium.scattering * uniformPhase;

		// Earth shadow 
		float tEarth = RaySphereIntersectNearest(P, SunDir, earthO + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET * UpVector, atmosphereParams.bottomRadius);
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

		// Dual scattering for multi scattering 
		float3 S = earthShadow * TransmittanceToSun * PhaseTimesScattering;

		// When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		// However, it is possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the orders has a really low contribution.
#define MULTI_SCATTERING_POWER_SERIE 1
#if MULTI_SCATTERING_POWER_SERIE==0
		// 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function of 1.0/(4*PI)
		result.MultiScatAs1 += throughput * medium.scattering * 1 * dt;
#else
		float3 MS = medium.scattering * 1;
		float3 MSint = (MS - MS * SampleTransmittance) / medium.extinction;
		result.MultiScatAs1 += throughput * MSint;
#endif

		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
		float3 Sint = (S - S * SampleTransmittance) / medium.extinction; // integrate along the current step segment 
		result.L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;

		tPrev = t;
	}
	
	if (tMax == tBottom && tBottom > 0.0)
	{
		// Account for bounced light off the earth
		float3 P = WorldPos + tBottom * WorldDir;
		float pHeight = length(P);

		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		float2 uv;
		LutTransmittanceParamsToUv(pHeight, SunZenithCosAngle, uv);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(Sampler_Bilinear_Clamp, uv, 0).rgb;

		const float NdotL = saturate(dot(normalize(UpVector), normalize(SunDir)));
		result.L += TransmittanceToSun * throughput * NdotL * atmosphereParams.groundAlbedo / PI;
	}
	return result;
}

groupshared float3 MultiScatAs1SharedMem[64];
groupshared float3 LSharedMem[64];

[numthreads(1, 1, 64)]
void skyAtmosphereMultiScatterLUTShader(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	float2 position = float2(dispatchThreadId.xy) + 0.5;
	float2 uv = position / float(SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES);
	uv = float2(FromSubUvsToUnit(uv.x, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES), FromSubUvsToUnit(uv.y, SKY_ATMOSPHERE_MULTISCATTERING_LUT_RES));
	
	float cosSunZenithAngle = uv.x * 2.0 - 1.0;
	float3 SunDir = float3(0.0, sqrt(saturate(1.0 - cosSunZenithAngle * cosSunZenithAngle)), cosSunZenithAngle);
	// We adjust again viewHeight according to PLANET_RADIUS_OFFSET to be in a valid range.
	float viewHeight = atmosphereParams.bottomRadius + saturate(uv.y + SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET) * (atmosphereParams.topRadius - atmosphereParams.bottomRadius - SKY_ATMOSPHERE_PLANET_RADIUS_OFFSET);

	float3 WorldPos = float3(0.0f, 0.0f, viewHeight);
	float3 WorldDir = float3(0.0f, 0.0f, 1.0f);
	
	const float SampleCountIni = 20; // a minimum set of step is required for accuracy unfortunately
	const float SphereSolidAngle = 4.0 * PI;
	const float IsotropicPhase = 1.0 / SphereSolidAngle;
	
	// Reference. Since there are many sample, it requires MULTI_SCATTERING_POWER_SERIE to be true for accuracy and to avoid divergences (see declaration for explanations)
#define SQRTSAMPLECOUNT 8
	const float sqrtSample = float(SQRTSAMPLECOUNT);
	float i = 0.5f + float(dispatchThreadId.z / SQRTSAMPLECOUNT);
	float j = 0.5f + float(dispatchThreadId.z - float((dispatchThreadId.z / SQRTSAMPLECOUNT) * SQRTSAMPLECOUNT));
	{
		float randA = i / sqrtSample;
		float randB = j / sqrtSample;
		float theta = 2.0f * PI * randA;
		float phi = acos(1.0f - 2.0f * randB); // uniform distribution https://mathworld.wolfram.com/SpherePointPicking.html
		//phi = PI * randB;						// bad non uniform
		float cosPhi = cos(phi);
		float sinPhi = sin(phi);
		float cosTheta = cos(theta);
		float sinTheta = sin(theta);
		WorldDir.x = cosTheta * sinPhi;
		WorldDir.y = sinTheta * sinPhi;
		WorldDir.z = cosPhi;
		MultiScatteringResult result = IntegrateMultiScattering(WorldPos, WorldDir, SunDir, SampleCountIni);

		MultiScatAs1SharedMem[dispatchThreadId.z] = result.MultiScatAs1 * SphereSolidAngle / (sqrtSample * sqrtSample);
		LSharedMem[dispatchThreadId.z] = result.L * SphereSolidAngle / (sqrtSample * sqrtSample);
	}
#undef SQRTSAMPLECOUNT

	GroupMemoryBarrierWithGroupSync();

	// 64 to 32
	if (dispatchThreadId.z < 32)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 32];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 32];
	}
	GroupMemoryBarrierWithGroupSync();

	// 32 to 16
	if (dispatchThreadId.z < 16)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 16];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 16];
	}
	GroupMemoryBarrierWithGroupSync();

	// 16 to 8 (16 is thread group min hardware size with intel, no sync required from there)
	if (dispatchThreadId.z < 8)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 8];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 8];
	}
	GroupMemoryBarrierWithGroupSync();
	if (dispatchThreadId.z < 4)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 4];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 4];
	}
	GroupMemoryBarrierWithGroupSync();
	if (dispatchThreadId.z < 2)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 2];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 2];
	}
	GroupMemoryBarrierWithGroupSync();
	if (dispatchThreadId.z < 1)
	{
		MultiScatAs1SharedMem[dispatchThreadId.z] += MultiScatAs1SharedMem[dispatchThreadId.z + 1];
		LSharedMem[dispatchThreadId.z] += LSharedMem[dispatchThreadId.z + 1];
	}
	GroupMemoryBarrierWithGroupSync();
	if (dispatchThreadId.z > 0)
		return;

	float3 MultiScatAs1 = MultiScatAs1SharedMem[0] * IsotropicPhase; // Equation 7 f_ms
	float3 InScatteredLuminance = LSharedMem[0] * IsotropicPhase; // Equation 5 L_2ndOrder

	// MultiScatAs1 represents the amount of luminance scattered as if the integral of scattered luminance over the sphere would be 1.
	//  - 1st order of scattering: one can ray-march a straight path as usual over the sphere. That is InScatteredLuminance.
	//  - 2nd order of scattering: the inscattered luminance is InScatteredLuminance at each of samples of fist order integration. Assuming a uniform phase function that is represented by MultiScatAs1,
	//  - 3nd order of scattering: the inscattered luminance is (InScatteredLuminance * MultiScatAs1 * MultiScatAs1)
	//  - etc.
#if	MULTI_SCATTERING_POWER_SERIE==0
	float3 MultiScatAs1SQR = MultiScatAs1 * MultiScatAs1;
	float3 L = InScatteredLuminance * (1.0 + MultiScatAs1 + MultiScatAs1SQR + MultiScatAs1 * MultiScatAs1SQR + MultiScatAs1SQR * MultiScatAs1SQR);
#else
	// For a serie, sum_{n=0}^{n=+inf} = 1 + r + r^2 + r^3 + ... + r^n = 1 / (1.0 - r), see https://en.wikipedia.org/wiki/Geometric_series 
	const float3 r = MultiScatAs1;
	const float3 SumOfAllMultiScatteringEventsContribution = 1.0f / (1.0 - r);
	float3 L = InScatteredLuminance * SumOfAllMultiScatteringEventsContribution; // Equation 10 Psi_ms
#endif

	RWTexture2D<float4> texture = ResourceDescriptorHeap[UAVIndex(atmosphereUniforms.multiScatterLutTexture)];
	texture[dispatchThreadId.xy] = float4(SKY_ATMOSPHERE_MULTISCAT_FACTOR * L, 1.0f);
}