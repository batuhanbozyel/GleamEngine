#pragma once
#include "Math/Color.h"
#include "Math/Vector3.h"

namespace Gleam {

static constexpr Float3 EarthRayleighScattering = { 0.005802f, 0.013558f, 0.033100f };
static constexpr Float3 EarthMieScattering = { 0.003996f, 0.003996f, 0.003996f };
static constexpr Float3 EarthMieExtinction = { 0.004440f, 0.004440f, 0.004440f };
static constexpr Float3 EarthMieAbsorption = EarthMieExtinction - EarthMieScattering;
static constexpr Float3 EarthAbsorptionExtinction = { 0.000650f, 0.001881f, 0.000085f };

GSTRUCT(Sun, "C418E66E-3D0A-47A8-965A-6E39BB4799A8", Serializable)
{
	GFIELD("1BB25490-F761-46E0-9CAD-A74DDFF9842B", Serializable)
	Color color = Color::white;

	GFIELD("EC8A7F1A-3430-428A-BB44-4BD07F297638", Serializable)
	float intensity = 10.0f;

	GFIELD("8E7961D8-59C5-4A1A-9E5B-7C193B78E825", Serializable)
	float angularDiameter = 0.5357f; // Angular diameter of sun to earth from sea level, see https://en.wikipedia.org/wiki/Solid_angle
};

GSTRUCT(Atmosphere, "387EB745-E0C1-4E78-ADE9-969FBC88E67A", Serializable)
{
	// Planet radii (in km)
	GFIELD("83789E86-6E4A-47E7-9BFD-C0D1972AE784", Serializable)
	float planetRadius = 6360.0f;
	GFIELD("1525011C-AD30-404A-8883-25490E7F10B6", Serializable)
	float atmosphereHeight = 100.0f;

	// Ground albedo (surface reflectance)
	GFIELD("ECE62A8B-19E5-4B9F-9006-E1FC6CEE6EE9", Serializable)
	Color groundAlbedo = Color::black;

	// Rayleigh scattering coefficients (wavelength dependent, RGB for earth-like atmosphere)
	GFIELD("04D20705-F0A1-4615-B4A5-EBB4E3ECF381", Serializable)
	float rayleighScatteringLength = Math::Length(EarthRayleighScattering);
	GFIELD("688504FC-80BB-4694-86C0-6EF5116F516A", Serializable)
	Color rayleighScattering = Color(EarthRayleighScattering / rayleighScatteringLength);
	GFIELD("4197C2BC-4D94-4013-AD90-55DD47D3119F", Serializable)
	float rayleighScaleHeight = 8.0f; // Exponential distribution scale height

	// Mie scattering coefficients (wavelength independent for aerosols)
	GFIELD("5B33F831-C52A-4B90-BF72-53B485632185", Serializable)
	float mieScatteringLength = Math::Length(EarthMieScattering);
	GFIELD("815CD29C-806C-4600-BF96-2E84823157DF", Serializable)
	Color mieScattering = Color(EarthMieScattering / mieScatteringLength);
	GFIELD("F23F0DDC-7257-46CA-BDA1-DC4C814889A1", Serializable)
	float mieAbsorptionLength = Math::Length(EarthMieAbsorption);
	GFIELD("70ABABFC-4B4B-44B1-BBAA-87AE936EB175", Serializable)
	Color mieAbsorption = Color(EarthMieAbsorption / mieAbsorptionLength);

	GFIELD("7C6E31DF-0353-47D0-8E9F-1E43B87B0A06", Serializable)
	float mieScaleHeight = 1.2f; // Exponential distribution scale height
	GFIELD("7ACAAFBD-9E24-4E24-9570-35E3F9A478BB", Serializable)
	float miePhaseG = 0.8f; // Anisotropy factor (Henyey-Greenstein phase function)

	// Ozone absorption (creates the blue sky effect by absorbing yellow/red)
	GFIELD("08145978-57C8-4F54-A6C7-D11C954C2419", Serializable)
	float absorptionLength = Math::Length(EarthAbsorptionExtinction);
	GFIELD("5DC799E0-67FF-4956-B96C-F62AAF4DA20B", Serializable)
	Color absorption = Color(EarthAbsorptionExtinction / absorptionLength);

	// Ozone absorption density profile (tent/trapezoid function)
	float absorptionDensity0LayerWidth = 25.0f; // Width of absorption layer (km)
	float absorptionDensity0ConstantTerm = -0.6666667f;
	float absorptionDensity0LinearTerm = 0.0666667f;
	float absorptionDensity1ConstantTerm = 2.6666667f;
	float absorptionDensity1LinearTerm = -0.0666667f;
};

GSTRUCT(SkyAtmosphere, "8BD6F0DA-CE0D-437A-AF4B-AE31F5B8FCA7", EntityComponent, Serializable)
{
	GFIELD("A8CAF395-ADCC-4053-B812-BFBBB70599C2", Serializable)
	Sun sun;

	GFIELD("9BF79E13-4A82-480C-B543-CF4D83004E4F", Serializable)
	Atmosphere atmosphere;
};

} // namespace Gleam
