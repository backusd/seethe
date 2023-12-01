#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class AddAtomsCR : public ChangeRequest
{
public:
	AddAtomsCR(const std::vector<AtomUUID>& uuids) noexcept :
		m_uuids(uuids),
		m_atomData(std::vector<AtomTPV>(uuids.size()))
	{
		ASSERT(uuids.size() > 0, "Invalid for uuids to be empty");

		LOG_WARN("AddAtomsCR(const std::vector<AtomUUID>& uuids) - Have not validated this constructor yet");
	}
	AddAtomsCR(AtomUUID uuid) noexcept :
		m_uuids{uuid},
		m_atomData{{}}
	{}
	AddAtomsCR(const AddAtomsCR&) noexcept = default;
	AddAtomsCR(AddAtomsCR&&) noexcept = default;
	AddAtomsCR& operator=(const AddAtomsCR&) noexcept = default;
	AddAtomsCR& operator=(AddAtomsCR&&) noexcept = default;
	virtual ~AddAtomsCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

private:
	std::vector<AtomUUID>	 m_uuids;
	std::vector<AtomTPV> m_atomData;
};
}