#include "AtomPositionCR.h"
#include "application/Application.h"
#include "utils/Log.h"

namespace seethe
{
void AtomPositionCR::Undo(Application* app) noexcept
{
	Atom& atom = app->GetSimulation().GetAtom(m_index);
	atom.position = m_positionInitial;
}
void AtomPositionCR::Redo(Application* app) noexcept
{
	Atom& atom = app->GetSimulation().GetAtom(m_index);
	atom.position = m_positionFinal;
}
}