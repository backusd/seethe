#include "AtomsMovedCR.h"
#include "application/Application.h"
#include "utils/Log.h"

using namespace DirectX;

namespace seethe
{
	void AtomsMovedCR::Undo(Application* app) noexcept
	{
		const XMFLOAT3 delta = { m_positionInitial.x - m_positionFinal.x, m_positionInitial.y - m_positionFinal.y, m_positionInitial.z - m_positionFinal.z };
		Simulation& simulation = app->GetSimulation();
		
		std::for_each(m_indices.begin(), m_indices.end(), 
			[&](const size_t& index) { simulation.MoveAtom(index, delta); });
	}
	void AtomsMovedCR::Redo(Application* app) noexcept
	{
		const XMFLOAT3 delta = { m_positionFinal.x - m_positionInitial.x, m_positionFinal.y - m_positionInitial.y, m_positionFinal.z - m_positionInitial.z };
		Simulation& simulation = app->GetSimulation();

		std::for_each(m_indices.begin(), m_indices.end(),
			[&](const size_t& index) { simulation.MoveAtom(index, delta); });
	}
}