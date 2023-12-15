#include "Simulation.h"

using namespace DirectX;

namespace seethe
{
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





}