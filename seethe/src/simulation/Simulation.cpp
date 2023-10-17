#include "Simulation.h"



namespace seethe
{
void Simulation::Update(const seethe::Timer& timer)
{
	float boxMaxX = m_boxLengthX / 2;
	float boxMaxY = m_boxLengthY / 2;
	float boxMaxZ = m_boxLengthZ / 2;

	float dt = timer.DeltaTime(); 
	
	for (auto& atom : m_atoms)
	{
		atom.position.x += atom.velocity.x * dt;
		atom.position.y += atom.velocity.y * dt;
		atom.position.z += atom.velocity.z * dt;

		// X
		if (atom.position.x + atom.radius > boxMaxX)
		{
			atom.position.x -= (atom.position.x + atom.radius - boxMaxX);
			atom.velocity.x *= -1;
		}

		if (atom.position.x - atom.radius < -boxMaxX)
		{
			atom.position.x -= (atom.position.x - atom.radius + boxMaxX);
			atom.velocity.x *= -1; 
		}

		// Y
		if (atom.position.y + atom.radius > boxMaxY)
		{
			atom.position.y -= (atom.position.y + atom.radius - boxMaxY);
			atom.velocity.y *= -1;
		}

		if (atom.position.y - atom.radius < -boxMaxY)
		{
			atom.position.y -= (atom.position.y - atom.radius + boxMaxY);
			atom.velocity.y *= -1;
		}

		// Z
		if (atom.position.z + atom.radius > boxMaxZ)
		{
			atom.position.z -= (atom.position.z + atom.radius - boxMaxZ);
			atom.velocity.z *= -1;
		}

		if (atom.position.z - atom.radius < -boxMaxZ)
		{
			atom.position.z -= (atom.position.z - atom.radius + boxMaxZ);
			atom.velocity.z *= -1;
		}
	}
}






}