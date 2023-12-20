#pragma once
#include "pch.h"

namespace seethe
{
// Max constant buffer size is 4096 float4's
// Our current SceneLighting class uses 2 float4's for non-Light data
// And the Light struct is 3 float4's
// (4096 - 2) / 3 = 1364.67
static constexpr unsigned int MaxLights = 1364;

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
public:
	ND constexpr DirectX::XMFLOAT4& AmbientLight() noexcept { return m_ambientLight; }
	constexpr void AmbientLight(const DirectX::XMFLOAT4& light) noexcept { m_ambientLight = light; }

	ND constexpr std::uint32_t NumDirectionalLights() const noexcept { return m_numDirectionalLights; }
	ND constexpr std::uint32_t NumPointLights() const noexcept { return m_numPointLights; }
	ND constexpr std::uint32_t NumSpotLights() const noexcept { return m_numSpotLights; }

	ND constexpr Light& GetDirectionalLight(size_t index) noexcept
	{
		ASSERT(index >= 0 && index < m_numDirectionalLights, "Invalid index");
		return m_lights[index];
	}
	ND constexpr Light& GetPointLight(size_t index) noexcept
	{
		ASSERT(index >= 0 && index < m_numPointLights, "Invalid index");
		return m_lights[m_numDirectionalLights + index];
	}
	ND constexpr Light& GetSpotLight(size_t index) noexcept
	{
		ASSERT(index >= 0 && index < m_numSpotLights, "Invalid index");
		return m_lights[index + m_numDirectionalLights + m_numPointLights];
	}

	constexpr void AddDirectionalLight(const DirectX::XMFLOAT3& strength, const DirectX::XMFLOAT3& direction) noexcept
	{
		MoveSpotLightsBack();
		MovePointLightsBack();
		m_lights[m_numDirectionalLights].Strength = strength;
		m_lights[m_numDirectionalLights].Direction = direction;
		++m_numDirectionalLights;
	}
	constexpr void AddPointLight(const DirectX::XMFLOAT3& strength, const DirectX::XMFLOAT3& position, float falloffStart, float falloffEnd) noexcept
	{
		MoveSpotLightsBack();

		size_t index = static_cast<size_t>(m_numDirectionalLights) + m_numPointLights;
		m_lights[index].Strength = strength;
		m_lights[index].Position = position;
		m_lights[index].FalloffStart = falloffStart;
		m_lights[index].FalloffEnd = falloffEnd;
		++m_numPointLights;
	}
	constexpr void AddSpotLight(const DirectX::XMFLOAT3& strength, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& direction, float falloffStart, float falloffEnd, float spotPower) noexcept
	{
		size_t index = static_cast<size_t>(m_numDirectionalLights) + m_numPointLights + m_numSpotLights;
		m_lights[index].Strength = strength;
		m_lights[index].Direction = direction;
		m_lights[index].Position = position;
		m_lights[index].FalloffStart = falloffStart;
		m_lights[index].FalloffEnd = falloffEnd;
		m_lights[index].SpotPower = spotPower;
		++m_numSpotLights;
	}

private:
	constexpr void MoveSpotLightsBack() noexcept
	{
		ASSERT(m_numDirectionalLights + m_numPointLights + m_numSpotLights < MaxLights, "Max lights have been reached");

		size_t start = static_cast<size_t>(m_numDirectionalLights) + m_numPointLights;
		size_t end = start + m_numSpotLights;
		for (size_t iii = end; iii > start; --iii)
			m_lights[iii] = m_lights[iii - 1];
	}
	constexpr void MovePointLightsBack() noexcept
	{
		size_t start = m_numDirectionalLights;
		size_t end = start + m_numPointLights;
		for (size_t iii = end; iii > start; --iii)
			m_lights[iii] = m_lights[iii - 1];
	}


	DirectX::XMFLOAT4 m_ambientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	std::uint32_t m_numDirectionalLights = 0;
	std::uint32_t m_numPointLights = 0;
	std::uint32_t m_numSpotLights = 0;
	std::uint32_t Pad0 = 0;

	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
	// are spot lights for a maximum of MaxLights per object.
	Light m_lights[MaxLights];
};

}