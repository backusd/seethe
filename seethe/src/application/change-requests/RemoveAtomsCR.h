#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class RemoveAtomsCR : public ChangeRequest
{
public:
	RemoveAtomsCR(const std::vector<std::tuple<size_t, AtomTPV>>& indicesAndData) noexcept :
		m_indicesAndData(indicesAndData)
	{
		ASSERT(m_indicesAndData.size() > 0, "Invalid for atom data to be empty");
	}
	RemoveAtomsCR(std::vector<std::tuple<size_t, AtomTPV>>&& indicesAndData) noexcept :
		m_indicesAndData(std::move(indicesAndData))
	{
		ASSERT(m_indicesAndData.size() > 0, "Invalid for atom data to be empty");
	}
	RemoveAtomsCR(size_t index, const AtomTPV& atomData) noexcept :
		m_indicesAndData{ { index, atomData } }
	{}
	RemoveAtomsCR(const RemoveAtomsCR&) noexcept = default;
	RemoveAtomsCR(RemoveAtomsCR&&) noexcept = default;
	RemoveAtomsCR& operator=(const RemoveAtomsCR&) noexcept = default;
	RemoveAtomsCR& operator=(RemoveAtomsCR&&) noexcept = default;
	virtual ~RemoveAtomsCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

private:
	std::vector<std::tuple<size_t, AtomTPV>> m_indicesAndData;
};
}