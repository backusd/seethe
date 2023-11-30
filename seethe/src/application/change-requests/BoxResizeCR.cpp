#include "BoxResizeCR.h"
#include "application\Application.h"


namespace seethe
{
void BoxResizeCR::Undo(Application* app) noexcept
{
	if (m_allowAtomsToRelocate)
		app->GetSimulation().SetAtoms(m_atomsInitial);

	app->SetBoxDimensions(m_initial, m_forceSidesToBeEqualInitial, m_allowAtomsToRelocate);
}
void BoxResizeCR::Redo(Application* app) noexcept
{
	if (m_allowAtomsToRelocate)
		app->GetSimulation().SetAtoms(m_atomsFinal);

	app->SetBoxDimensions(m_final, m_forceSidesToBeEqualFinal, m_allowAtomsToRelocate);
}
}