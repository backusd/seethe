#include "BoxResizeCR.h"
#include "application\Application.h"


namespace seethe
{
void BoxResizeCR::Undo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();
	simulation.SetDimensions(m_initial, m_allowAtomsToRelocate);

	if (m_allowAtomsToRelocate)
		simulation.SetAtoms(m_atomsInitial);

	SimulationSettings& settings = app->GetSimulationSettings();
	settings.boxDimensions = m_initial;
	settings.forceSidesToBeEqual = m_forceSidesToBeEqualInitial;
}
void BoxResizeCR::Redo(Application* app) noexcept
{
	Simulation& simulation = app->GetSimulation();
	simulation.SetDimensions(m_final, m_allowAtomsToRelocate);

	if (m_allowAtomsToRelocate)
		simulation.SetAtoms(m_atomsFinal);

	SimulationSettings& settings = app->GetSimulationSettings();
	settings.boxDimensions = m_final;
	settings.forceSidesToBeEqual = m_forceSidesToBeEqualFinal;
}
}