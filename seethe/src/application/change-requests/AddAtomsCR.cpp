#include "AddAtomsCR.h"
#include "application/Application.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
	void AddAtomsCR::Undo(Application* app) noexcept
	{
		app->GetSimulation().RemoveLastAtoms(m_atomData.size());
	}
	void AddAtomsCR::Redo(Application* app) noexcept
	{
		app->GetSimulation().AddAtoms(m_atomData);
	}
}

#pragma pop_macro("AddAtom") // See note above for 'AddAtom'