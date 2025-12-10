#include "Common.hlsli"
#include "ShaderTypes.h"
#include "SkyAtmosphereCommon.hlsli"

PUSH_CONSTANT(Gleam::SkyAtmosphereTransmittanceLutUniforms, uniforms);

#pragma fragment skyAtmosphereTransmittanceLUTShader

float4 skyAtmosphereTransmittanceLUTShader(FScreenVertexOutput IN) : SV_TARGET
{
	float viewHeight, viewZenithCosAngle;
	UvToLutTransmittanceParams(atmosphereParams, IN.texCoord, viewHeight, viewZenithCosAngle);

	float3 rayOrigin = float3(0.0, 0.0, viewHeight);
	float3 rayDirection = float3(0.0, sqrt(1.0 - viewZenithCosAngle * viewZenithCosAngle), viewZenithCosAngle);

	const bool ground = false;
	const float sampleCountIni = 40.0f;	// Can go a low as 10 sample but energy lost starts to be visible.
	const float depthBufferValue = -1.0;
	const bool variableSampleCount = false;
	const bool mieRayPhase = false;
	float3 transmittance = exp(-IntegrateScatteredLuminance(pixPos, rayOrigin, rayDirection, sun_direction, atmosphereParams, ground, sampleCountIni, depthBufferValue, variableSampleCount, mieRayPhase).OpticalDepth);

	// Optical depth to transmittance
	return float4(transmittance, 1.0f);
}