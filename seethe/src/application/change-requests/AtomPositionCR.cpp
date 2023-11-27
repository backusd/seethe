#include "AtomPositionCR.h"
#include "application/Application.h"
#include "utils/Log.h"

namespace seethe
{
void AtomPositionCR::Undo(Application* app) noexcept
{
	try
	{
		Atom& atom = app->GetSimulation().GetAtomByUUID(m_atomUUID);
		atom.position = m_positionInitial;
	}
	catch (const std::runtime_error& e)
	{
		LOG_ERROR("ERROR: AtomPositionCR::Undo action failed with message: {}", e.what());
	}
}
void AtomPositionCR::Redo(Application* app) noexcept
{
	try
	{
		Atom& atom = app->GetSimulation().GetAtomByUUID(m_atomUUID);
		atom.position = m_positionFinal;
	}
	catch (const std::runtime_error& e)
	{
		LOG_ERROR("ERROR: AtomPositionCR::Redo action failed with message: {}", e.what());
	}
}
}