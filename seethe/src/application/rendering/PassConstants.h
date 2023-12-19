#pragma once
#include "pch.h"


namespace seethe
{
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float Pad0 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	//	seethe::SceneLighting Lighting = {};

	//	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
	//
	//	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	//	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	//	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	//	// are spot lights for a maximum of MaxLights per object.
	//	seethe::Light Lights[MaxLights];
};
}