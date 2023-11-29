#include "AddAtomCR.h"
#include "application/Application.h"

// Windows defines an 'AddAtom' macro, so we undefine it here so we can use it for a member function on Simulation
#pragma push_macro("AddAtom")
#undef AddAtom

namespace seethe
{
	void AddAtomCR::Undo(Application* app) noexcept
	{
		Simulation& simulation = app->GetSimulation();

		// Save the current values
		Atom& atom = simulation.GetAtomByUUID(m_uuid);
		m_position = atom.position;
		m_velocity = atom.velocity;
		m_type = atom.type;

		// Delete the atom
		simulation.RemoveAtomByUUID(m_uuid);

		// Inform the application
		app->AtomsRemoved();
	}
	void AddAtomCR::Redo(Application* app) noexcept
	{
		Simulation& simulation = app->GetSimulation();

		// Create the atom
		const Atom& atom = simulation.AddAtom(m_type, m_position, m_velocity);

		// Save the new uuid - not guaranteed to be the same as before
		m_uuid = atom.uuid;

		// Make the atom selected
		simulation.SelectAtomByUUID(m_uuid);

		// Inform the application (NOTE: This MUST come last so that the SimulationWindow can
		//						   see that the atom is selected)
		app->AtomsAdded();
	}
}

#pragma pop_macro("AddAtom") // See note above for 'AddAtom'