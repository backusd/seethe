#include "RemoveAtomsCR.h"
#include "application/Application.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
void RemoveAtomsCR::Undo(Application* app) noexcept
{
	app->GetSimulation().AddAtoms(m_indicesAndData);
}
void RemoveAtomsCR::Redo(Application* app) noexcept
{
	std::vector<size_t> indices;
	indices.reserve(m_indicesAndData.size());
	std::for_each(m_indicesAndData.begin(), m_indicesAndData.end(), [&indices](const std::tuple<size_t, AtomTPV>& tup) { indices.push_back(std::get<0>(tup)); });
	app->GetSimulation().RemoveAtoms(indices);
}
}

#pragma pop_macro("AddAtom") // See note above for 'AddAtom'