#pragma once
#include "pch.h"

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

private:
	std::vector<Atom> m_atoms;
};
}

#pragma pop_macro("AddAtom")