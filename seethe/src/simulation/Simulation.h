#pragma once
#include "pch.h"
#include "utils/Timer.h"
#include "utils/Log.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function
#pragma push_macro("AddAtom")
#undef AddAtom

using AtomUUID = size_t;

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

// Forward declare so that we can make Simulation a friend
class Simulation;

// This is a helper struct for grouping the pieces of data that are necessary when
// adding/removing multiple atoms. Instead of having to all a method like 'AddAtom'
// multiple times, we can call a method like 'AddAtoms' with a vector of AtomTPV
struct AtomTPV
{
	AtomTPV() noexcept = default;
	AtomTPV(AtomType _type, const DirectX::XMFLOAT3& _position, const DirectX::XMFLOAT3& _velocity) noexcept :
		type(_type), position(_position), velocity(_velocity)
	{}
	AtomType			type = AtomType::HYDROGEN;
	DirectX::XMFLOAT3	position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3	velocity = { 0.0f, 0.0f, 0.0f };
};

class Atom
{
public:
	Atom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) noexcept;
	Atom(const Atom& rhs) noexcept = default;
	Atom& operator=(const Atom&) noexcept = default;
	Atom(Atom&&) noexcept = default;
	Atom& operator=(Atom&&) noexcept = default;

	ND static float RadiusOf(AtomType type) noexcept;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	float radius;
	AtomType type;
	AtomUUID uuid;

private:
	static AtomUUID m_nextUUID;

	friend Simulation;
};

class Simulation
{
public:
	void AddAtom(const Atom& atom) { m_atoms.push_back(atom); }
	const Atom& AddAtom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) noexcept { return m_atoms.emplace_back(type, position, velocity); }
	const Atom& AddAtom(const AtomTPV& data) noexcept { return AddAtom(data.type, data.position, data.velocity); }
	std::vector<AtomUUID> AddAtoms(const std::vector<AtomTPV>& data) noexcept;
	void RemoveAtomByIndex(size_t index) noexcept;
	void RemoveAtomsByIndex(std::vector<size_t>& indices) noexcept;
	void RemoveAtomsByIndex(const std::vector<size_t>& indices) noexcept;
	void RemoveAtomByUUID(AtomUUID uuid) noexcept;
	void RemoveAtomsByUUID(const std::vector<AtomUUID>& uuids) noexcept;
	void RemoveAllSelectedAtoms() noexcept;

	void Update(const seethe::Timer& timer);
	
	ND inline const std::vector<Atom>& GetAtoms() const noexcept { return m_atoms; }
	ND inline std::vector<Atom>& GetAtoms() noexcept { return m_atoms; }
	ND Atom& GetAtomByUUID(AtomUUID uuid);
	ND const Atom& GetAtomByUUID(AtomUUID uuid) const;
	ND Atom& GetAtomByIndex(size_t index) noexcept { return m_atoms[index]; }
	ND const Atom& GetAtomByIndex(size_t index) const noexcept { return m_atoms[index]; }
	ND inline DirectX::XMFLOAT3 GetDimensions() const noexcept { return { m_boxMaxX * 2, m_boxMaxY * 2, m_boxMaxZ * 2 }; }
	ND inline DirectX::XMFLOAT3 GetDimensionMaxs() const noexcept { return { m_boxMaxX, m_boxMaxY, m_boxMaxZ }; }
	ND float GetMaxAxisAlignedDistanceFromOrigin() const noexcept;
	ND const std::vector<size_t>& GetSelectedAtomIndices() const noexcept { return m_selectedAtomIndices; }
	ND const DirectX::XMFLOAT3& GetSelectedAtomsCenter() const noexcept { return m_selectedAtomsCenter; }

	inline void SetAtoms(const std::vector<Atom>& atoms) noexcept { m_atoms = atoms; }
	inline void SetAtoms(std::vector<Atom>&& atoms) noexcept { m_atoms = std::move(atoms); }
	inline bool SetDimensions(float lengthXYZ, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengthXYZ, lengthXYZ, lengthXYZ, allowAtomsToRelocate); }
	bool SetDimensions(const DirectX::XMFLOAT3& lengths, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengths.x, lengths.y, lengths.z, allowAtomsToRelocate); }
	bool SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate = true) noexcept;

	ND constexpr inline bool IsPlaying() const noexcept { return m_isPlaying; }
	constexpr void StartPlaying() noexcept { m_isPlaying = true; }
	constexpr void StopPlaying() noexcept { m_isPlaying = false; }

	inline void SelectAtomByIndex(size_t index) noexcept { m_selectedAtomIndices.push_back(index); UpdateSelectedAtomsCenter(); }
	void SelectAtomByUUID(AtomUUID uuid) noexcept;
	ND inline bool AtomAtIndexIsSelected(size_t index) const noexcept { return m_selectedAtomIndices.cend() != std::find(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), index); }
	ND inline bool AtomWithUUIDIsSelected(AtomUUID uuid) const noexcept { return m_selectedAtomIndices.cend() != std::find_if(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), [this, uuid](const size_t& index) { return m_atoms[index].uuid == uuid; }); }
	ND inline bool AtLeastOneAtomWithUUIDIsSelected(const std::vector<size_t>& uuids) const noexcept { return uuids.cend() != std::find_if(uuids.cbegin(), uuids.cend(), [this](const size_t& uuid) { return AtomWithUUIDIsSelected(uuid); }); }
	inline void ClearSelectedAtoms() noexcept { m_selectedAtomIndices.clear(); UpdateSelectedAtomsCenter(); }
	void UnselectAtomByIndex(size_t index) noexcept;
	void UnselectAtomsByIndex(const std::vector<size_t> indices) noexcept;
	void UnselectAtomByUUID(AtomUUID uuid) noexcept;
	void UnselectAtomsByUUID(const std::vector<AtomUUID>& uuids) noexcept;

	inline void ReseatNextUUID() noexcept { Atom::m_nextUUID = m_atoms.size() > 0 ? m_atoms.back().uuid + 1 : 0; }

private:
	bool DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept;

	void DecrementSelectedIndicesBeyondIndex(size_t index) noexcept;
	void UpdateSelectedAtomsCenter() noexcept;

	std::vector<Atom> m_atoms = {};
	std::vector<size_t> m_selectedAtomIndices;
	DirectX::XMFLOAT3 m_selectedAtomsCenter = { 0.0f, 0.0f, 0.0f };

	// Note: the box will have dimensions [-m_boxMax*, m_boxMax*] for each axis
	float m_boxMaxX = 10.0f;
	float m_boxMaxY = 10.0f;
	float m_boxMaxZ = 10.0f;

	bool m_isPlaying = false;
};
}

#pragma pop_macro("AddAtom")