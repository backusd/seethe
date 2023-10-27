#include "Simulation.h"
#include "utils/Log.h"

using namespace DirectX;

namespace seethe
{
void Simulation::Update(const seethe::Timer& timer)
{
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




}