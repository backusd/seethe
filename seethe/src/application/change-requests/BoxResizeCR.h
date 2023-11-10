#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class BoxResizeCR : public ChangeRequest
{
public:
	BoxResizeCR(const DirectX::XMFLOAT3& initial, const DirectX::XMFLOAT3& final, bool allowRelocation, 
		bool forceSidesToBeEqualInitial, bool forceSidesToBeEqualFinal,
		const std::vector<Atom>& atomsInitial = {}, const std::vector<Atom>& atomsFinal = {}) noexcept :
		m_initial(initial), m_final(final), m_allowAtomsToRelocate(allowRelocation), 
		m_forceSidesToBeEqualInitial(forceSidesToBeEqualInitial),
		m_forceSidesToBeEqualFinal(forceSidesToBeEqualFinal),
		m_atomsInitial(atomsInitial),
		m_atomsFinal(atomsFinal)
	{}
	BoxResizeCR(const BoxResizeCR&) noexcept = default;
	BoxResizeCR(BoxResizeCR&&) noexcept = default;
	BoxResizeCR& operator=(const BoxResizeCR&) noexcept = default;
	BoxResizeCR& operator=(BoxResizeCR&&) noexcept = default;
	virtual ~BoxResizeCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

	DirectX::XMFLOAT3 m_initial;
	DirectX::XMFLOAT3 m_final;
	bool m_allowAtomsToRelocate;
	bool m_forceSidesToBeEqualInitial;
	bool m_forceSidesToBeEqualFinal;
	
	std::vector<Atom> m_atomsInitial;
	std::vector<Atom> m_atomsFinal;
};
}