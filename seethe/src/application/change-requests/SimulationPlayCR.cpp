#include "SimulationPlayCR.h"
#include "application\Application.h"


namespace seethe
{
void SimulationPlayCR::Undo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();

	// First, keep track of where the atoms currently are
	m_final = simulation.GetAtoms();

	// Replace all atoms to their initial locations
	simulation.SetAtoms(m_initial);
}
void SimulationPlayCR::Redo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();
	simulation.SetAtoms(m_final);
}
}