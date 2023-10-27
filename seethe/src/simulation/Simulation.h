#pragma once
#include "pch.h"
#include "utils/Timer.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
struct Atom
{
	DirectX::XMFLOAT3 position = {};
	DirectX::XMFLOAT3 velocity = {};
	float radius = 1.0f;
};

class Simulation
{
public:
	void AddAtom(const Atom& atom) { m_atoms.push_back(atom); }
	void AddAtom(const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}, float radius = 1.0f) { m_atoms.push_back({position, velocity, radius}); }

	ND inline const std::vector<Atom>& Atoms() const noexcept { return m_atoms; }

	void Update(const seethe::Timer& timer);

	ND inline DirectX::XMFLOAT3 GetDimensions() const noexcept { return { m_boxMaxX, m_boxMaxY, m_boxMaxZ }; }

	inline bool SetDimensions(float lengthXYZ, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengthXYZ, lengthXYZ, lengthXYZ, allowAtomsToRelocate); }
	bool SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate = true) noexcept;

private:
	bool DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept;

	std::vector<Atom> m_atoms = {};

	// Note: the box will have dimensions [-m_boxMax*, m_boxMax*] for each axis
	float m_boxMaxX = 10.0f;
	float m_boxMaxY = 10.0f;
	float m_boxMaxZ = 10.0f;
};
}

#pragma pop_macro("AddAtom")