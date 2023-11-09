#pragma once
#include "pch.h"
#include "ChangeRequest.h"


namespace seethe
{
class BoxResizeCR : public ChangeRequest
{
public:
	BoxResizeCR(const DirectX::XMFLOAT3& initial, const DirectX::XMFLOAT3& final, bool allowRelocation, 
		bool forceSidesToBeEqualInitial, bool forceSidesToBeEqualFinal) noexcept :
		m_initial(initial), m_final(final), m_allowAtomsToRelocate(allowRelocation), 
		m_forceSidesToBeEqualInitial(forceSidesToBeEqualInitial),
		m_forceSidesToBeEqualFinal(forceSidesToBeEqualFinal)
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
};
}