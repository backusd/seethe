#pragma once
#include "pch.h"
#include "utils/Timer.h"
#include "utils/Log.h"
#include "utils/Event.h"

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
	inline void AddAtom(const Atom& atom) noexcept { m_atoms.push_back(atom); InvokeHandlers(m_atomsAddedHandlers); }
	const Atom& AddAtom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) noexcept;
	const Atom& AddAtom(const AtomTPV& data) noexcept { return AddAtom(data.type, data.position, data.velocity); }
	std::vector<AtomUUID> AddAtoms(const std::vector<AtomTPV>& data) noexcept;
	void RemoveAtomByIndex(size_t index, bool invokeHandlers = true) noexcept;
	void RemoveAtomsByIndex(std::vector<size_t>& indices) noexcept;
	void RemoveAtomsByIndex(const std::vector<size_t>& indices) noexcept;
	void RemoveAtomByUUID(AtomUUID uuid, bool invokeHandlers = true) noexcept;
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
	ND const DirectX::XMFLOAT3& GetSelectedAtomsCenter() noexcept 
	{ 
		if (m_isPlaying)
			UpdateSelectedAtomsCenter(); 
		return m_selectedAtomsCenter; 
	}

	void SetAtoms(const std::vector<Atom>& atoms) noexcept;
	void SetAtoms(std::vector<Atom>&& atoms) noexcept;
	inline bool SetDimensions(float lengthXYZ, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengthXYZ, lengthXYZ, lengthXYZ, allowAtomsToRelocate); }
	bool SetDimensions(const DirectX::XMFLOAT3& lengths, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengths.x, lengths.y, lengths.z, allowAtomsToRelocate); }
	bool SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate = true) noexcept;

	ND constexpr inline bool IsPlaying() const noexcept { return m_isPlaying; }
	inline void StartPlaying() noexcept { m_isPlaying = true; InvokeHandlers(m_simulationStartedHandlers); }
	inline void StopPlaying() noexcept { m_isPlaying = false; UpdateSelectedAtomsCenter(); InvokeHandlers(m_simulationStoppedHandlers); }

	inline void SelectAtomByIndex(size_t index) noexcept { m_selectedAtomIndices.push_back(index); UpdateSelectedAtomsCenter(); InvokeHandlers(m_selectedAtomsChangedHandlers); }
	void SelectAtomByUUID(AtomUUID uuid) noexcept;
	ND inline bool AtomAtIndexIsSelected(size_t index) const noexcept { return m_selectedAtomIndices.cend() != std::find(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), index); }
	ND inline bool AtLeastOneAtomWithIndexIsSelected(const std::vector<size_t>& indices) const noexcept { return indices.cend() != std::find_if(indices.cbegin(), indices.cend(), [this](const size_t& index) { return AtomAtIndexIsSelected(index); }); }
	ND inline bool AtomWithUUIDIsSelected(AtomUUID uuid) const noexcept { return m_selectedAtomIndices.cend() != std::find_if(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), [this, uuid](const size_t& index) { return m_atoms[index].uuid == uuid; }); }
	ND inline bool AtLeastOneAtomWithUUIDIsSelected(const std::vector<AtomUUID>& uuids) const noexcept { return uuids.cend() != std::find_if(uuids.cbegin(), uuids.cend(), [this](const AtomUUID& uuid) { return AtomWithUUIDIsSelected(uuid); }); }
	inline void ClearSelectedAtoms() noexcept { m_selectedAtomIndices.clear(); UpdateSelectedAtomsCenter(); InvokeHandlers(m_selectedAtomsChangedHandlers); }
	void UnselectAtomByIndex(size_t index, bool invokeHandlers = true) noexcept;
	void UnselectAtomsByIndex(const std::vector<size_t> indices) noexcept;
	void UnselectAtomByUUID(AtomUUID uuid, bool invokeHandlers = true) noexcept;
	void UnselectAtomsByUUID(const std::vector<AtomUUID>& uuids) noexcept;

	inline void ReseatNextUUID() noexcept { Atom::m_nextUUID = m_atoms.size() > 0 ? m_atoms.back().uuid + 1 : 0; }

	constexpr void UpdateSelectedAtomsCenter() noexcept;

	// Handlers
	inline void RegisterBoxSizeChangedHandler(const EventHandler& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	inline void RegisterBoxSizeChangedHandler(EventHandler&& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	inline void RegisterSelectedAtomsChangedHandler(const EventHandler& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	inline void RegisterSelectedAtomsChangedHandler(EventHandler&& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	inline void RegisterAtomsAddedHandler(const EventHandler& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	inline void RegisterAtomsAddedHandler(EventHandler&& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	inline void RegisterAtomsRemovedHandler(const EventHandler& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }
	inline void RegisterAtomsRemovedHandler(EventHandler&& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }
	inline void RegisterSimulationStartedHandler(const EventHandler& handler) noexcept { m_simulationStartedHandlers.push_back(handler); }
	inline void RegisterSimulationStartedHandler(EventHandler&& handler) noexcept { m_simulationStartedHandlers.push_back(handler); }
	inline void RegisterSimulationStoppedHandler(const EventHandler& handler) noexcept { m_simulationStoppedHandlers.push_back(handler); }
	inline void RegisterSimulationStoppedHandler(EventHandler&& handler) noexcept { m_simulationStoppedHandlers.push_back(handler); }

private:
	bool DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept;

	void DecrementSelectedIndicesBeyondIndex(size_t index) noexcept;


	std::vector<Atom> m_atoms = {};
	std::vector<size_t> m_selectedAtomIndices;
	DirectX::XMFLOAT3 m_selectedAtomsCenter = { 0.0f, 0.0f, 0.0f };

	// Event handlers
	EventHandlers m_boxSizeChangedHandlers;
	EventHandlers m_selectedAtomsChangedHandlers;
	EventHandlers m_atomsAddedHandlers;
	EventHandlers m_atomsRemovedHandlers;
	EventHandlers m_simulationStartedHandlers;
	EventHandlers m_simulationStoppedHandlers;

	// Note: the box will have dimensions [-m_boxMax*, m_boxMax*] for each axis
	float m_boxMaxX = 10.0f;
	float m_boxMaxY = 10.0f;
	float m_boxMaxZ = 10.0f;

	bool m_isPlaying = false;
};
}

#pragma pop_macro("AddAtom")