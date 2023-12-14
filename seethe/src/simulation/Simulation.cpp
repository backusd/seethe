#include "Simulation.h"

using namespace DirectX;

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
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

float Atom::RadiusOf(AtomType type) noexcept
{
	return AtomicRadii[static_cast<size_t>(type) - 1];
}

Atom::Atom(AtomType _type, const XMFLOAT3& _position, const XMFLOAT3& _velocity) noexcept :
	type(_type),
	position(_position),
	velocity(_velocity),
	radius(AtomicRadii[static_cast<int>(_type) - 1])
{}
Atom& Simulation::AddAtom(AtomType type, const XMFLOAT3& position, const XMFLOAT3& velocity) noexcept
{
	Atom& atom = m_atoms.emplace_back(type, position, velocity);
	InvokeHandlers(m_atomsAddedHandlers);
	return atom;
}
Atom& Simulation::AddAtom(const AtomTPV& data, size_t index) noexcept
{
	if (index == m_atoms.size())
		return AddAtom(data);

	auto iter = m_atoms.insert(m_atoms.begin() + index, { data.type, data.position, data.velocity });
	InvokeHandlers(m_atomsAddedHandlers); 
	return *iter;
}
std::vector<Atom*> Simulation::AddAtoms(const std::vector<std::tuple<size_t, AtomTPV>>& indicesAndData) noexcept
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
std::vector<Atom*> Simulation::AddAtoms(const std::vector<AtomTPV>& data) noexcept
{
	std::vector<Atom*> atoms;
	atoms.reserve(data.size());

	std::for_each(data.begin(), data.end(), [&atoms, this](const AtomTPV& d) { atoms.push_back(&AddAtom(d)); });

	InvokeHandlers(m_atomsAddedHandlers);
	return atoms;
}

void Simulation::RemoveAtoms(std::vector<size_t>& indices) noexcept
{
	bool oneIsSelected = AtLeastOneAtomWithIndexIsSelected(indices);

	// Must first sort the indices because it is only safe to erase largest to smallest
	std::sort(indices.begin(), indices.end(), std::greater<size_t>());
	std::for_each(indices.begin(), indices.end(), [this](const size_t& index) { RemoveAtom(index, false); });
	
	if (oneIsSelected)
		InvokeHandlers(m_selectedAtomsChangedHandlers);
	InvokeHandlers(m_atomsRemovedHandlers);
}
void Simulation::RemoveAtoms(const std::vector<size_t>& indices) noexcept
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
void Simulation::RemoveAtom(size_t index, bool invokeHandlers) noexcept
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
void Simulation::RemoveLastAtoms(size_t count) noexcept
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
void Simulation::RemoveAllSelectedAtoms() noexcept
{
	// Make a copy of the indices here because the remove methods will attempt to modify m_selectedAtomIndices
	std::vector<size_t> copy = m_selectedAtomIndices;
	RemoveAtoms(copy);
	ASSERT(m_selectedAtomIndices.size() == 0, "Something went wrong - this should be empty");
}
void Simulation::DecrementSelectedIndicesBeyondIndex(size_t index) noexcept
{
	std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [index](size_t& _index) {
		if (_index > index)
			_index -= 1;
		}
	);
}

void Simulation::SetAtoms(const std::vector<Atom>& atoms) noexcept 
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
void Simulation::SetAtoms(std::vector<Atom>&& atoms) noexcept 
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

void Simulation::Update(const seethe::Timer& timer)
{
	if (!m_isPlaying) return;

	float dt = timer.DeltaTime(); 
	
	for (auto& atom : m_atoms)
	{
		atom.position.x += atom.velocity.x * dt;
		atom.position.y += atom.velocity.y * dt;
		atom.position.z += atom.velocity.z * dt;

		// X
		if (atom.position.x + atom.radius > m_boxMaxX)
		{
			atom.position.x -= (atom.position.x + atom.radius - m_boxMaxX);
			atom.velocity.x *= -1;
		}

		if (atom.position.x - atom.radius < -m_boxMaxX)
		{
			atom.position.x -= (atom.position.x - atom.radius + m_boxMaxX);
			atom.velocity.x *= -1; 
		}

		// Y
		if (atom.position.y + atom.radius > m_boxMaxY)
		{
			atom.position.y -= (atom.position.y + atom.radius - m_boxMaxY);
			atom.velocity.y *= -1;
		}

		if (atom.position.y - atom.radius < -m_boxMaxY)
		{
			atom.position.y -= (atom.position.y - atom.radius + m_boxMaxY);
			atom.velocity.y *= -1;
		}

		// Z
		if (atom.position.z + atom.radius > m_boxMaxZ)
		{
			atom.position.z -= (atom.position.z + atom.radius - m_boxMaxZ);
			atom.velocity.z *= -1;
		}

		if (atom.position.z - atom.radius < -m_boxMaxZ)
		{
			atom.position.z -= (atom.position.z - atom.radius + m_boxMaxZ);
			atom.velocity.z *= -1;
		}
	}
}

bool Simulation::SetDimensions(float lengthX, float lengthY, float lengthZ, bool allowAtomsToRelocate) noexcept
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
bool Simulation::DimensionUpdateTryRelocation(float& position, float radius, float newMax, bool allowRelocation) noexcept
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
float Simulation::GetMaxAxisAlignedDistanceFromOrigin() const noexcept
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

void Simulation::SelectAtom(size_t index, bool unselectAllOthersFirst) noexcept
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
void Simulation::UnselectAtom(size_t index, bool invokeHandlers) noexcept
{
	ASSERT(index < m_atoms.size(), "Index too large");
	std::erase(m_selectedAtomIndices, index); 
	UpdateSelectedAtomsCenter();

	if (invokeHandlers)
		InvokeHandlers(m_selectedAtomsChangedHandlers);
}
void Simulation::UnselectAtoms(const std::vector<size_t> indices) noexcept
{
	for (size_t index : indices)
	{
		ASSERT(index < m_atoms.size(), "Index too large"); 
		std::erase(m_selectedAtomIndices, index); 
	}
	UpdateSelectedAtomsCenter(); 
	InvokeHandlers(m_selectedAtomsChangedHandlers);
}

constexpr void Simulation::UpdateSelectedAtomsCenter() noexcept
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

}

#pragma pop_macro("AddAtom")