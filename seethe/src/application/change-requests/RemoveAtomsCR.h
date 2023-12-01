#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class RemoveAtomsCR : public ChangeRequest
{
public:
	RemoveAtomsCR(const std::vector<AtomTPV>& atomData) noexcept :
		m_uuids(std::vector<AtomUUID>(atomData.size())),
		m_atomData(atomData)
	{
		ASSERT(m_atomData.size() > 0, "Invalid for atom data to be empty");
	}
	RemoveAtomsCR(std::vector<AtomTPV>&& atomData) noexcept :
		m_uuids(std::vector<AtomUUID>(atomData.size())),
		m_atomData(std::move(atomData))
	{
		ASSERT(m_atomData.size() > 0, "Invalid for atom data to be empty");
	}
	RemoveAtomsCR(const AtomTPV& atomData) noexcept :
		m_uuids{ 0 }, // Initialize with 0 so the size of the vectors match
		m_atomData{ atomData }
	{}
	RemoveAtomsCR(AtomTPV&& atomData) noexcept :
		m_uuids{ 0 }, // Initialize with 0 so the size of the vectors match
		m_atomData{ std::move(atomData) }
	{}
	RemoveAtomsCR(const RemoveAtomsCR&) noexcept = default;
	RemoveAtomsCR(RemoveAtomsCR&&) noexcept = default;
	RemoveAtomsCR& operator=(const RemoveAtomsCR&) noexcept = default;
	RemoveAtomsCR& operator=(RemoveAtomsCR&&) noexcept = default;
	virtual ~RemoveAtomsCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

private:
	std::vector<AtomUUID>	m_uuids;
	std::vector<AtomTPV>	m_atomData;
};
}