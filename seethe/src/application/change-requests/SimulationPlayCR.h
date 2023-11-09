#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class SimulationPlayCR : public ChangeRequest
{
public:
	SimulationPlayCR(const std::vector<Atom>& initial, const std::vector<Atom> & final = {}) noexcept :
		m_initial(initial), m_final(final)
	{}
	SimulationPlayCR(const SimulationPlayCR&) noexcept = default;
	SimulationPlayCR(SimulationPlayCR&&) noexcept = default;
	SimulationPlayCR& operator=(const SimulationPlayCR&) noexcept = default;
	SimulationPlayCR& operator=(SimulationPlayCR&&) noexcept = default;
	virtual ~SimulationPlayCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

	std::vector<Atom> m_initial;
	std::vector<Atom> m_final;
};
}