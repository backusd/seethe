#pragma once
#include "pch.h"

// Max constant buffer size is 4096 float4's
// Our current SceneLighting class uses 2 float4's for non-Light data
// And the Light struct is 3 float4's
// (4096 - 2) / 3 = 1364.67
static constexpr int MaxLights = 1364;

namespace seethe
{
struct Light
{
	DirectX::XMFLOAT3   Strength = { 0.5f, 0.5f, 0.5f };
	float               FalloffStart = 1.0f;                // point/spot light only
	DirectX::XMFLOAT3   Direction = { 0.0f, -1.0f, 0.0f };  // directional/spot light only
	float               FalloffEnd = 10.0f;                 // point/spot light only
	DirectX::XMFLOAT3   Position = { 0.0f, 0.0f, 0.0f };    // point/spot light only
	float               SpotPower = 64.0f;                  // spot light only
};

struct SceneLighting
{
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	std::uint32_t NumDirectionalLights = 0;
	std::uint32_t NumPointLights = 0;
	std::uint32_t NumSpotLights = 0;
	std::uint32_t Pad0 = 0;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light Lights[MaxLights];
};

}