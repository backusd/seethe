#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class AddAtomsCR : public ChangeRequest
{
public:
	AddAtomsCR(const std::vector<AtomTPV>& data) noexcept :
		m_atomData(data)
	{
		ASSERT(data.size() > 0, "Invalid for data to be empty");
	}
	AddAtomsCR(std::vector<AtomTPV>&& data) noexcept :
		m_atomData(std::move(data))
	{
		ASSERT(m_atomData.size() > 0, "Invalid for data to be empty");
	}
	AddAtomsCR(const AtomTPV& data) noexcept :
		m_atomData{ data }
	{}
	AddAtomsCR(const AddAtomsCR&) noexcept = default;
	AddAtomsCR(AddAtomsCR&&) noexcept = default;
	AddAtomsCR& operator=(const AddAtomsCR&) noexcept = default;
	AddAtomsCR& operator=(AddAtomsCR&&) noexcept = default;
	virtual ~AddAtomsCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

private:
	std::vector<AtomTPV> m_atomData;
};
}