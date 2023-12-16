#pragma once
#include "pch.h"
#include "utils/Timer.h"
#include "utils/Log.h"
#include "utils/Event.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
static constexpr unsigned int AtomTypeCount = 10;
static constexpr std::array AtomNames = { "Hydrogen", "Helium", "Lithium", "Beryllium", "Boron", 
										  "Carbon", "Nitrogen", "Oxygen", "Flourine", "Neon" };

static constexpr std::array<float, AtomTypeCount> AtomicRadii = {
	0.5f,
	0.6f,
	0.7f,
	0.8f,
	0.9f,
	1.0f,
	1.1f,
	1.2f,
	1.3f,
	1.4f
};

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
	constexpr AtomTPV() noexcept = default; 
	constexpr AtomTPV(AtomType _type, const DirectX::XMFLOAT3& _position, const DirectX::XMFLOAT3& _velocity) noexcept :
		type(_type), position(_position), velocity(_velocity)
	{}
	AtomType			type = AtomType::HYDROGEN;
	DirectX::XMFLOAT3	position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3	velocity = { 0.0f, 0.0f, 0.0f };
};

class Atom
{
public:
	constexpr Atom(AtomType _type, const DirectX::XMFLOAT3& _position = {}, const DirectX::XMFLOAT3& _velocity = {}) noexcept :
		type(_type),
		position(_position),
		velocity(_velocity),
		radius(AtomicRadii[static_cast<int>(_type) - 1])
	{}
	constexpr Atom(const Atom& rhs) noexcept = default;
	constexpr Atom& operator=(const Atom&) noexcept = default;
	constexpr Atom(Atom&&) noexcept = default;
	constexpr Atom& operator=(Atom&&) noexcept = default;

	ND static constexpr float RadiusOf(AtomType type) noexcept
	{
		return AtomicRadii[static_cast<size_t>(type) - 1];
	}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 velocity;
	float radius;
	AtomType type;

private:
	friend Simulation;
};

class Simulation
{
public:
	void Update(const seethe::Timer& timer);

	constexpr void AddAtom(const Atom& atom) noexcept { m_atoms.push_back(atom); InvokeHandlers(m_atomsAddedHandlers); }
	constexpr Atom& AddAtom(AtomType type, const DirectX::XMFLOAT3& position = {}, const DirectX::XMFLOAT3& velocity = {}) noexcept
	{
		Atom& atom = m_atoms.emplace_back(type, position, velocity);
		InvokeHandlers(m_atomsAddedHandlers);
		return atom;
	}
	constexpr Atom& AddAtom(const AtomTPV& data) noexcept { return AddAtom(data.type, data.position, data.velocity); }
	constexpr Atom& AddAtom(const AtomTPV& data, size_t index) noexcept
	{
		if (index == m_atoms.size())
			return AddAtom(data);

		auto iter = m_atoms.insert(m_atoms.begin() + index, { data.type, data.position, data.velocity });
		InvokeHandlers(m_atomsAddedHandlers);
		return *iter;
	}
	constexpr std::vector<Atom*> AddAtoms(const std::vector<std::tuple<size_t, AtomTPV>>& indicesAndData) noexcept
	{
		// NOTE: We make the assumption here that if we are adding multiple atoms at specific indices, then the index requested
		//       is the FINAL index. Therefore, we must add them in order from smallest to largest index, otherwise, adding larger
		//		 ones first would lead to those atoms being pushed back further when atoms with smaller indices are added.
		std::vector<Atom*> atoms;
		atoms.reserve(indicesAndData.size());

		// Make a copy so we can sort it
		std::vector<std::tuple<size_t, AtomTPV>> data = indicesAndData;
		std::ranges::sort(data, [](const std::tuple<size_t, AtomTPV>& lhs, const std::tuple<size_t, AtomTPV>& rhs) { return std::get<0>(lhs) < std::get<0>(rhs); });

		std::for_each(data.begin(), data.end(), [&atoms, this](const std::tuple<size_t, AtomTPV>& tup)
			{
				size_t index = std::get<0>(tup);
				const AtomTPV& tpv = std::get<1>(tup);

				if (index == m_atoms.size())
				{
					Atom& a = m_atoms.emplace_back(tpv.type, tpv.position, tpv.velocity);
					atoms.push_back(&a);
				}
				else
				{
					auto iter = m_atoms.insert(m_atoms.begin() + index, { tpv.type, tpv.position, tpv.velocity });
					atoms.push_back(&(*iter));
				}
			});

		InvokeHandlers(m_atomsAddedHandlers);
		return atoms;
	}
	constexpr std::vector<Atom*> AddAtoms(const std::vector<AtomTPV>& data) noexcept
	{
		std::vector<Atom*> atoms;
		atoms.reserve(data.size());

		std::for_each(data.begin(), data.end(), [&atoms, this](const AtomTPV& d) { atoms.push_back(&AddAtom(d)); });

		InvokeHandlers(m_atomsAddedHandlers);
		return atoms;
	}
	constexpr void RemoveAtom(size_t index, bool invokeHandlers = true) noexcept
	{
		ASSERT(index < m_atoms.size(), "Index too large");

		// Remove the index from selected list (if it exists)
		if (AtomIsSelected(index))
			UnselectAtom(index, invokeHandlers);

		// Decrement all of the selected indices that lie beyond the atom being removed
		DecrementSelectedIndicesBeyondIndex(index);

		// Erase the atom
		m_atoms.erase(m_atoms.begin() + index);

		// Invoke the handlers
		if (invokeHandlers)
			InvokeHandlers(m_atomsRemovedHandlers);
	}
	constexpr void RemoveAtoms(std::vector<size_t>& indices) noexcept
	{
		bool oneIsSelected = AtLeastOneAtomWithIndexIsSelected(indices);

		// Must first sort the indices because it is only safe to erase largest to smallest
		std::sort(indices.begin(), indices.end(), std::greater<size_t>());
		std::for_each(indices.begin(), indices.end(), [this](const size_t& index) { RemoveAtom(index, false); });

		if (oneIsSelected)
			InvokeHandlers(m_selectedAtomsChangedHandlers);
		InvokeHandlers(m_atomsRemovedHandlers);
	}
	constexpr void RemoveAtoms(const std::vector<size_t>& indices) noexcept
	{
		// If the indices are not sorted, then we must make a copy because the vector is const which means it can't be sorted
		if (!std::is_sorted(indices.begin(), indices.end(), std::greater<size_t>()))
		{
			std::vector<size_t> copy = indices;
			RemoveAtoms(copy);
		}
		else
		{
			bool oneIsSelected = AtLeastOneAtomWithIndexIsSelected(indices);
			std::for_each(indices.begin(), indices.end(), [this](const size_t& index) { RemoveAtom(index, false); });
			if (oneIsSelected)
				InvokeHandlers(m_selectedAtomsChangedHandlers);
			InvokeHandlers(m_atomsRemovedHandlers);
		}
	}
	constexpr void RemoveLastAtoms(size_t count) noexcept
	{
		ASSERT(count <= m_atoms.size(), "Count is too large");

		// Construct a vector of indices where we start with the largest index and work backwards until we have #count indices
		std::vector<size_t> indices;
		indices.reserve(count);

		size_t last = m_atoms.size() - count;
		for (size_t iii = m_atoms.size() - 1; iii >= last; --iii)
			indices.push_back(iii);

		RemoveAtoms(indices);
	}
	constexpr void RemoveAllSelectedAtoms() noexcept
	{
		// Make a copy of the indices here because the remove methods will attempt to modify m_selectedAtomIndices
		std::vector<size_t> copy = m_selectedAtomIndices;
		RemoveAtoms(copy);
		ASSERT(m_selectedAtomIndices.size() == 0, "Something went wrong - this should be empty");
	}
	
	// See here for article on 'deducing this' pattern: https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/
	template <class Self>
	ND constexpr auto&& GetAtoms(this Self&& self) noexcept { return std::forward<Self>(self).m_atoms; }
	template <class Self>
	ND constexpr auto&& GetAtom(this Self&& self, size_t index) noexcept { return std::forward<Self>(self).m_atoms[index]; }
	template <class Self>
	ND constexpr auto&& GetSelectedAtomIndices(this Self&& self) noexcept { return std::forward<Self>(self).m_selectedAtomIndices; }

	ND constexpr DirectX::XMFLOAT3 GetDimensions() const noexcept { return { m_boxMaxX * 2, m_boxMaxY * 2, m_boxMaxZ * 2 }; }
	ND constexpr DirectX::XMFLOAT3 GetDimensionMaxs() const noexcept { return { m_boxMaxX, m_boxMaxY, m_boxMaxZ }; }
	ND constexpr float GetMaxAxisAlignedDistanceFromOrigin() const noexcept
	{
		float max = 0.0f;

		for (const auto& atom : m_atoms)
		{
			float x = std::abs(atom.position.x) + atom.radius;
			float y = std::abs(atom.position.y) + atom.radius;
			float z = std::abs(atom.position.z) + atom.radius;

			max = std::max(max, std::max(x, std::max(y, z)));
		}

		return max;
	}
	ND constexpr const DirectX::XMFLOAT3& GetSelectedAtomsCenter() noexcept 
	{ 
		if (m_isPlaying)
			UpdateSelectedAtomsCenter(); 
		return m_selectedAtomsCenter; 
	}

	constexpr void SetAtoms(const std::vector<Atom>& atoms) noexcept
	{
		// In probably most cases, we are calling this function because of an undo/redo action and therefore,
		// don't have to worry about atom selection because the atoms themselves are not changing (they are 
		// just changing location). However, it's possible we use this method as a means of completely resetting
		// the entire simulation, in which case, we need to make sure that the selected indices don't exceed the 
		// new number of atoms. To be safe, we are just going to clear the selected indices because the actual 
		// selected indices probably wouldn't make sense anyways
		if (atoms.size() != m_atoms.size())
		{
			m_selectedAtomIndices.clear();
			InvokeHandlers(m_selectedAtomsChangedHandlers);
		}

		m_atoms = atoms;
		InvokeHandlers(m_atomsAddedHandlers);
	}
	constexpr void SetAtoms(std::vector<Atom>&& atoms) noexcept
	{
		// In probably most cases, we are calling this function because of an undo/redo action and therefore,
		// don't have to worry about atom selection because the atoms themselves are not changing (they are 
		// just changing location). However, it's possible we use this method as a means of completely resetting
		// the entire simulation, in which case, we need to make sure that the selected indices don't exceed the 
		// new number of atoms. To be safe, we are just going to clear the selected indices because the actual 
		// selected indices probably wouldn't make sense anyways
		if (atoms.size() != m_atoms.size())
		{
			m_selectedAtomIndices.clear();
			InvokeHandlers(m_selectedAtomsChangedHandlers);
		}

		m_atoms = std::move(atoms);
		InvokeHandlers(m_atomsAddedHandlers);
	}
	constexpr bool SetDimensions(float lengthXYZ, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengthXYZ, lengthXYZ, lengthXYZ, allowAtomsToRelocate); }
	constexpr bool SetDimensions(const DirectX::XMFLOAT3& lengths, bool allowAtomsToRelocate = true) noexcept { return SetDimensions(lengths.x, lengths.y, lengths.z, allowAtomsToRelocate); }
	constexpr bool SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate = true) noexcept
	{
		float newMaxX = lengthX / 2;
		float newMaxY = lengthY / 2;
		float newMaxZ = lengthZ / 2;

		// If we are making the box smaller in any dimension, then we must ensure that we don't no atoms
		// end up outside the box and if so, that we are allowed to relocate the atoms
		if (newMaxX < m_boxMaxX)
		{
			for (auto& atom : m_atoms)
			{
				if (!DimensionUpdateTryRelocation(atom.position.x, atom.radius, newMaxX, allowAtomsToRelocate))
				{
					LOG_ERROR("Failed to set new simulation box dimensions ({}, {}, {}) because one or more atoms would be outside the box and the forceAtomsToRelocate flag is 'false'", lengthX, lengthY, lengthZ);
					return false;
				}
			}
		}
		if (newMaxY < m_boxMaxY)
		{
			for (auto& atom : m_atoms)
			{
				if (!DimensionUpdateTryRelocation(atom.position.y, atom.radius, newMaxY, allowAtomsToRelocate))
				{
					LOG_ERROR("Failed to set new simulation box dimensions ({}, {}, {}) because one or more atoms would be outside the box and the forceAtomsToRelocate flag is 'false'", lengthX, lengthY, lengthZ);
					return false;
				}
			}
		}
		if (newMaxZ < m_boxMaxZ)
		{
			for (auto& atom : m_atoms)
			{
				if (!DimensionUpdateTryRelocation(atom.position.z, atom.radius, newMaxZ, allowAtomsToRelocate))
				{
					LOG_ERROR("Failed to set new simulation box dimensions ({}, {}, {}) because one or more atoms would be outside the box and the forceAtomsToRelocate flag is 'false'", lengthX, lengthY, lengthZ);
					return false;
				}
			}
		}

		// At this point, any and all relocation should have already happened, so we can just update the box values
		m_boxMaxX = newMaxX;
		m_boxMaxY = newMaxY;
		m_boxMaxZ = newMaxZ;

		InvokeHandlers(m_boxSizeChangedHandlers);
		return true;
	}

	ND constexpr inline bool IsPlaying() const noexcept { return m_isPlaying; }
	constexpr void StartPlaying() noexcept { m_isPlaying = true; InvokeHandlers(m_simulationStartedHandlers); }
	constexpr void StopPlaying() noexcept { m_isPlaying = false; UpdateSelectedAtomsCenter(); InvokeHandlers(m_simulationStoppedHandlers); }

	constexpr void SelectAtom(size_t index, bool unselectAllOthersFirst = false) noexcept
	{
		ASSERT(index < m_atoms.size(), "Index is too large");
		if (!AtomIsSelected(index))
		{
			if (unselectAllOthersFirst)
				ClearSelectedAtoms();

			m_selectedAtomIndices.push_back(index);
			UpdateSelectedAtomsCenter();
			InvokeHandlers(m_selectedAtomsChangedHandlers);
		}
	}
	constexpr void SelectAtom(const Atom& atom, bool unselectAllOthersFirst = false) noexcept { SelectAtom(IndexOf(atom), unselectAllOthersFirst); }
	ND constexpr bool AtomIsSelected(const Atom& atom) const noexcept { return AtomIsSelected(IndexOf(atom)); }
	ND constexpr bool AtomIsSelected(size_t index) const noexcept { return m_selectedAtomIndices.cend() != std::find(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), index); }
	ND constexpr bool AtLeastOneAtomWithIndexIsSelected(const std::vector<size_t>& indices) const noexcept { return indices.cend() != std::find_if(indices.cbegin(), indices.cend(), [this](const size_t& index) { return AtomIsSelected(index); }); }
	constexpr void ClearSelectedAtoms() noexcept { m_selectedAtomIndices.clear(); UpdateSelectedAtomsCenter(); InvokeHandlers(m_selectedAtomsChangedHandlers); }
	constexpr void UnselectAtom(size_t index, bool invokeHandlers = true) noexcept
	{
		ASSERT(index < m_atoms.size(), "Index too large");
		std::erase(m_selectedAtomIndices, index);
		UpdateSelectedAtomsCenter();

		if (invokeHandlers)
			InvokeHandlers(m_selectedAtomsChangedHandlers);
	}
	constexpr void UnselectAtom(const Atom& atom, bool InvokeHandlers = true) noexcept { UnselectAtom(IndexOf(atom), InvokeHandlers); }
	constexpr void UnselectAtoms(const std::vector<size_t> indices) noexcept
	{
		for (size_t index : indices)
		{
			ASSERT(index < m_atoms.size(), "Index too large");
			std::erase(m_selectedAtomIndices, index);
		}
		UpdateSelectedAtomsCenter();
		InvokeHandlers(m_selectedAtomsChangedHandlers);
	}

	constexpr void UpdateSelectedAtomsCenter() noexcept
	{
		m_selectedAtomsCenter = { 0.0f, 0.0f, 0.0f };
		size_t count = m_selectedAtomIndices.size();

		if (count > 0)
		{
			const float factor = 1.0f / count;

			for (size_t iii : m_selectedAtomIndices)
			{
				const Atom& atom = m_atoms[iii];

				m_selectedAtomsCenter.x += (atom.position.x * factor);
				m_selectedAtomsCenter.y += (atom.position.y * factor);
				m_selectedAtomsCenter.z += (atom.position.z * factor);
			}
		}
	}

	constexpr void MoveSelectedAtomsX(float delta) noexcept
	{
		if (MoveSelectedAtomsXIsInBounds(delta))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, delta](const size_t& index) { m_atoms[index].position.x += delta; });
			UpdateSelectedAtomsCenter();
		}
	}
	constexpr void MoveSelectedAtomsY(float delta) noexcept
	{
		if (MoveSelectedAtomsYIsInBounds(delta))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, delta](const size_t& index) { m_atoms[index].position.y += delta; });
			UpdateSelectedAtomsCenter();
		}
	}
	constexpr void MoveSelectedAtomsZ(float delta) noexcept
	{
		if (MoveSelectedAtomsZIsInBounds(delta))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, delta](const size_t& index) { m_atoms[index].position.z += delta; });
			UpdateSelectedAtomsCenter();
		}
	}
	constexpr void MoveSelectedAtomsXY(float deltaX, float deltaY) noexcept
	{
		if (MoveSelectedAtomsXIsInBounds(deltaX) && MoveSelectedAtomsYIsInBounds(deltaY))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, deltaX, deltaY](const size_t& index) { m_atoms[index].position.x += deltaX; m_atoms[index].position.y += deltaY; });
			UpdateSelectedAtomsCenter();
		}
	}
	constexpr void MoveSelectedAtomsXZ(float deltaX, float deltaZ) noexcept
	{
		if (MoveSelectedAtomsXIsInBounds(deltaX) && MoveSelectedAtomsZIsInBounds(deltaZ))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, deltaX, deltaZ](const size_t& index) { m_atoms[index].position.x += deltaX; m_atoms[index].position.z += deltaZ; });
			UpdateSelectedAtomsCenter();
		}
	}
	constexpr void MoveSelectedAtomsYZ(float deltaY, float deltaZ) noexcept
	{
		if (MoveSelectedAtomsYIsInBounds(deltaY) && MoveSelectedAtomsZIsInBounds(deltaZ))
		{
			std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [this, deltaY, deltaZ](const size_t& index) { m_atoms[index].position.y += deltaY; m_atoms[index].position.z += deltaZ; });
			UpdateSelectedAtomsCenter();
		}
	}

	// Handlers
	constexpr void RegisterBoxSizeChangedHandler(const EventHandler& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	constexpr void RegisterBoxSizeChangedHandler(EventHandler&& handler) noexcept { m_boxSizeChangedHandlers.push_back(handler); }
	constexpr void RegisterSelectedAtomsChangedHandler(const EventHandler& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	constexpr void RegisterSelectedAtomsChangedHandler(EventHandler&& handler) noexcept { m_selectedAtomsChangedHandlers.push_back(handler); }
	constexpr void RegisterAtomsAddedHandler(const EventHandler& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	constexpr void RegisterAtomsAddedHandler(EventHandler&& handler) noexcept { m_atomsAddedHandlers.push_back(handler); }
	constexpr void RegisterAtomsRemovedHandler(const EventHandler& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }
	constexpr void RegisterAtomsRemovedHandler(EventHandler&& handler) noexcept { m_atomsRemovedHandlers.push_back(handler); }
	constexpr void RegisterSimulationStartedHandler(const EventHandler& handler) noexcept { m_simulationStartedHandlers.push_back(handler); }
	constexpr void RegisterSimulationStartedHandler(EventHandler&& handler) noexcept { m_simulationStartedHandlers.push_back(handler); }
	constexpr void RegisterSimulationStoppedHandler(const EventHandler& handler) noexcept { m_simulationStoppedHandlers.push_back(handler); }
	constexpr void RegisterSimulationStoppedHandler(EventHandler&& handler) noexcept { m_simulationStoppedHandlers.push_back(handler); }

	ND constexpr size_t IndexOf(const Atom& atom) const noexcept { return static_cast<size_t>(std::distance(m_atoms.data(), &atom)); }

private:
	ND constexpr bool DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept
	{
		// Check positive max
		if (position + radius > newMax)
		{
			if (allowRelocation)
				position = newMax - radius;
			else
				return false;
		}
		// Check negative max
		else if (position - radius < -newMax)
		{
			if (allowRelocation)
				position = -newMax + radius;
			else
				return false;
		}

		return true;
	}

	constexpr void DecrementSelectedIndicesBeyondIndex(size_t index) noexcept
	{
		std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [index](size_t& _index) {
			if (_index > index)
				_index -= 1;
			}
		);
	}

	ND constexpr bool MoveSelectedAtomsXIsInBounds(float delta) const noexcept
	{
		for (size_t index : m_selectedAtomIndices)
		{
			const Atom& atom = m_atoms[index];
			float f = atom.position.x + delta;
			if (f + atom.radius > m_boxMaxX || f - atom.radius < -m_boxMaxX)
				return false;
		}
		return true;
	}
	ND constexpr bool MoveSelectedAtomsYIsInBounds(float delta) const noexcept
	{
		for (size_t index : m_selectedAtomIndices)
		{
			const Atom& atom = m_atoms[index];
			float f = atom.position.y + delta;
			if (f + atom.radius > m_boxMaxY || f - atom.radius < -m_boxMaxY)
				return false;
		}
		return true;
	}
	ND constexpr bool MoveSelectedAtomsZIsInBounds(float delta) const noexcept
	{
		for (size_t index : m_selectedAtomIndices)
		{
			const Atom& atom = m_atoms[index];
			float f = atom.position.z + delta;
			if (f + atom.radius > m_boxMaxZ || f - atom.radius < -m_boxMaxZ)
				return false;
		}
		return true;
	}

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