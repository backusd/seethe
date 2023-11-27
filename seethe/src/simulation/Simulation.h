#pragma once
#include "pch.h"
#include "utils/Timer.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
static constexpr unsigned int AtomTypeCount = 10;
static constexpr std::array AtomNames = { "Hydrogen", "Helium", "Lithium", "Beryllium", "Boron", 
										  "Carbon", "Nitrogen", "Oxygen", "Flourine", "Neon" };

enum class AtomType
{
	HYDROGEN = 1,
	HELIUM = 2,
	LITHIUM = 3,
	BERYLLIUM = 4,
	BORON = 5,
	CARBON = 6,
	NITROGEN = 7,
	OXYGEN = 8,
	FLOURINE = 9,
	NEON = 10
};

class Atom
{
public:
	Atom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) noexcept;
	Atom(const Atom& rhs) noexcept = default;
	Atom& operator=(const Atom&) noexcept = default;
	Atom(Atom&&) noexcept = default;
	Atom& operator=(Atom&&) noexcept = default;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	float radius;
	AtomType type;
	unsigned int uuid;

private:
	static unsigned int m_nextUUID;
};

class Simulation
{
public:
	void AddAtom(const Atom& atom) { m_atoms.push_back(atom); }
	void AddAtom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) { m_atoms.emplace_back(type, position, velocity); }

	void Update(const seethe::Timer& timer);
	
	ND inline const std::vector<Atom>& GetAtoms() const noexcept { return m_atoms; }
	ND inline std::vector<Atom>& GetAtoms() noexcept { return m_atoms; }
	ND Atom& GetAtomByUUID(unsigned int uuid);
	ND const Atom& GetAtomByUUID(unsigned int uuid) const;
	ND inline DirectX::XMFLOAT3 GetDimensions() const noexcept { return { m_boxMaxX * 2, m_boxMaxY * 2, m_boxMaxZ * 2 }; }
	ND inline DirectX::XMFLOAT3 GetDimensionMaxs() const noexcept { return { m_boxMaxX, m_boxMaxY, m_boxMaxZ }; }
	ND float GetMaxAxisAlignedDistanceFromOrigin() const noexcept;

	inline void SetAtoms(const std::vector<Atom>& atoms) noexcept { m_atoms = atoms; }
	inline void SetAtoms(std::vector<Atom>&& atoms) noexcept { m_atoms = std::move(atoms); }
	inline bool SetDimensions(float lengthXYZ, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengthXYZ, lengthXYZ, lengthXYZ, allowAtomsToRelocate); }
	bool SetDimensions(const DirectX::XMFLOAT3& lengths, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengths.x, lengths.y, lengths.z, allowAtomsToRelocate); }
	bool SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate = true) noexcept;

	ND constexpr inline bool IsPlaying() const noexcept { return m_isPlaying; }
	constexpr void StartPlaying() noexcept { m_isPlaying = true; }
	constexpr void StopPlaying() noexcept { m_isPlaying = false; }

private:
	bool DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept;

	std::vector<Atom> m_atoms = {};

	// Note: the box will have dimensions [-m_boxMax*, m_boxMax*] for each axis
	float m_boxMaxX = 10.0f;
	float m_boxMaxY = 10.0f;
	float m_boxMaxZ = 10.0f;

	bool m_isPlaying = false;
};
}

#pragma pop_macro("AddAtom")