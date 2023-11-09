#include "SimulationPlayCR.h"
#include "application\Application.h"


namespace seethe
{
void SimulationPlayCR::Undo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();
	simulation.SetAtoms(m_initial);
}
void SimulationPlayCR::Redo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();
	simulation.SetAtoms(m_final);
}
}