#include "AtomVelocityCR.h"
#include "application/Application.h"
#include "utils/Log.h"

namespace seethe
{
	void AtomVelocityCR::Undo(Application* app) noexcept
	{
		try
		{
			Atom& atom = app->GetSimulation().GetAtomByUUID(m_atomUUID);
			atom.velocity = m_velocityInitial;
		}
		catch (const std::runtime_error& e)
		{
			LOG_ERROR("ERROR: AtomVelocityCR::Undo action failed with message: {}", e.what());
		}
	}
	void AtomVelocityCR::Redo(Application* app) noexcept
	{
		try
		{
			Atom& atom = app->GetSimulation().GetAtomByUUID(m_atomUUID);
			atom.velocity = m_velocityFinal;
		}
		catch (const std::runtime_error& e)
		{
			LOG_ERROR("ERROR: AtomVelocityCR::Redo action failed with message: {}", e.what());
		}
	}
}