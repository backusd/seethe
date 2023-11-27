#include "Simulation.h"

using namespace DirectX;

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

size_t Atom::m_nextUUID = 0;

Atom::Atom(AtomType _type, const XMFLOAT3& _position, const XMFLOAT3& _velocity) noexcept :
	type(_type),
	position(_position),
	velocity(_velocity),
	radius(AtomicRadii[static_cast<int>(_type) - 1]),
	uuid(m_nextUUID++)
{}

void Simulation::RemoveAtomByIndex(size_t index) noexcept
{
	ASSERT(index < m_atoms.size(), "Index too large");

	// Remove the index from selected list (if it exists)
	if (AtomAtIndexIsSelected(index)) 
		UnselectAtomByIndex(index);

	// Decrement all of the selected indices that lie beyond the atom being removed
	DecrementSelectedIndicesBeyondIndex(index);

	// Erase the atom
	m_atoms.erase(m_atoms.begin() + index);
}
void Simulation::RemoveAtomByUUID(size_t uuid) noexcept
{
	auto itr = std::find_if(m_atoms.cbegin(), m_atoms.cend(),
		[uuid](const Atom& atom) { return atom.uuid == uuid; });

	if (itr != m_atoms.cend())
	{
		if (AtomWithUUIDIsSelected(uuid))
			UnselectAtomByUUID(uuid);

		DecrementSelectedIndicesBeyondIndex(itr - m_atoms.cbegin());

		m_atoms.erase(itr);
	}
	else
	{
		LOG_ERROR("ERROR: Simulation::RemoveAtomByUUID failed to find atom with uuid: {}", uuid);
	}
}
void Simulation::DecrementSelectedIndicesBeyondIndex(size_t index) noexcept
{
	std::for_each(m_selectedAtomIndices.begin(), m_selectedAtomIndices.end(), [index](size_t& _index) {
		if (_index > index)
			_index -= 1;
		}
	);
}

Atom& Simulation::GetAtomByUUID(size_t uuid)
{
	// First do a simple check to see if the atom w/ uuid is at index uuid
	if (uuid < m_atoms.size() && m_atoms[uuid].uuid == uuid)
		return m_atoms[uuid];

	// If not, then search for it
	auto pos = std::find_if(m_atoms.begin(), m_atoms.end(), 
		[uuid](const Atom& atom) -> bool
		{
			return atom.uuid == uuid;
		});

	if (pos != m_atoms.end())
		return *pos;

	throw std::runtime_error(std::format("Could not find atom with uuid: {}", uuid));
}
const Atom& Simulation::GetAtomByUUID(size_t uuid) const
{
	// First do a simple check to see if the atom w/ uuid is at index uuid
	if (uuid < m_atoms.size() && m_atoms[uuid].uuid == uuid) 
		return m_atoms[uuid]; 

	// If not, then search for it
	auto pos = std::find_if(m_atoms.cbegin(), m_atoms.cend(), 
		[uuid](const Atom& atom) -> bool 
		{
			return atom.uuid == uuid; 
		});

	if (pos != m_atoms.cend()) 
		return *pos;

	throw std::runtime_error(std::format("Could not find atom with uuid: {}", uuid)); 
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

void Simulation::SelectAtomByUUID(size_t uuid) noexcept
{
	if (!AtomWithUUIDIsSelected(uuid))
	{
		for (size_t iii = 0; iii < m_atoms.size(); ++iii)
		{
			if (m_atoms[iii].uuid == uuid)
			{
				m_selectedAtomIndices.push_back(iii);
				return;
			}
		}

		LOG_ERROR("ERROR: Simulation::SelectAtomByUUID failed to find atom with uuid: {}", uuid);
	}
}
void Simulation::UnselectAtomByUUID(size_t uuid) noexcept
{
	auto itr = std::find_if(m_selectedAtomIndices.cbegin(), m_selectedAtomIndices.cend(), 
		[this, uuid](const size_t& index) { return m_atoms[index].uuid == uuid; });

	if (itr != m_selectedAtomIndices.cend())
		m_selectedAtomIndices.erase(itr);
	else
		LOG_ERROR("ERROR: Simulation::UnselectAtomByUUID failed to find atom with uuid: {}", uuid);
}

}