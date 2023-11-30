#include "AddAtomCR.h"
#include "application/Application.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
	void AddAtomCR::Undo(Application* app) noexcept
	{
		// Save the current values
		// NOTE: GetAtomByUUID() will throw if the uuid is not found
		try
		{
			Atom& atom = app->GetSimulation().GetAtomByUUID(m_uuid);
			m_position = atom.position;
			m_velocity = atom.velocity;
			m_type = atom.type;
		}
		catch (const std::runtime_error& e)
		{
			LOG_ERROR("ERROR: AddAtomCR::Undo action failed with message: {}", e.what());

			// These are not guaranteed to have accurate values, so just default them
			m_position = { 0.0f, 0.0f, 0.0f };
			m_velocity = { 0.0f, 0.0f, 0.0f };
			m_type = AtomType::HYDROGEN;
		}

		app->RemoveAtomByUUID(m_uuid);
	}
	void AddAtomCR::Redo(Application* app) noexcept
	{
		// Add the atom and make it selected
		const Atom& atom = app->AddAtom(m_type, m_position, m_velocity);
		m_uuid = atom.uuid;
		app->SelectAtomByUUID(m_uuid);
	}
}

#pragma pop_macro("AddAtom") // See note above for 'AddAtom'