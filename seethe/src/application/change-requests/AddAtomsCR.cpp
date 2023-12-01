#include "AddAtomsCR.h"
#include "application/Application.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
	void AddAtomsCR::Undo(Application* app) noexcept
	{
		ASSERT(m_uuids.size() == m_atomData.size(), "Sizes should always match");

		for (size_t iii = 0; iii < m_uuids.size(); ++iii)
		{
			// Save the current values
			// NOTE: GetAtomByUUID() will throw if the uuid is not found
			try
			{
				Atom& atom = app->GetSimulation().GetAtomByUUID(m_uuids[iii]);
				m_atomData[iii] = { atom.type, atom.position, atom.velocity };
			}
			catch (const std::runtime_error& e)
			{
				LOG_ERROR("ERROR: AddAtomCR::Undo action failed with message: {}", e.what());
			}
		}

		// Specify false here so we don't add an additional CR to the undo stack
		app->RemoveAtomsByUUID(m_uuids, false);
	}
	void AddAtomsCR::Redo(Application* app) noexcept
	{
		ASSERT(m_uuids.size() == m_atomData.size(), "Sizes should always match");

		// Specify false here so we don't add an additional CR to the undo stack
		m_uuids = app->AddAtoms(m_atomData, false);
	}
}

#pragma pop_macro("AddAtom") // See note above for 'AddAtom'