#include "AtomVelocityCR.h"
#include "application/Application.h"
#include "utils/Log.h"

namespace seethe
{
void AtomVelocityCR::Undo(Application* app) noexcept
{
	Atom& atom = app->GetSimulation().GetAtom(m_index);
	atom.velocity = m_velocityInitial;
}
void AtomVelocityCR::Redo(Application* app) noexcept
{
	Atom& atom = app->GetSimulation().GetAtom(m_index);
	atom.velocity = m_velocityFinal;
}
}