#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "application/ui/Enums.h"

namespace seethe
{
class AtomsMovedCR : public ChangeRequest
{
public:
	AtomsMovedCR(const std::vector<size_t>& atomIndices, const DirectX::XMFLOAT3& initialPosition, const DirectX::XMFLOAT3& finalPosition) noexcept :
		m_indices(atomIndices),
		m_positionInitial(initialPosition),
		m_positionFinal(finalPosition)
	{}
	AtomsMovedCR(size_t index, const DirectX::XMFLOAT3& initialPosition, const DirectX::XMFLOAT3& finalPosition) noexcept :
		m_indices{index},
		m_positionInitial(initialPosition),
		m_positionFinal(finalPosition)
	{}
	AtomsMovedCR(const AtomsMovedCR&) noexcept = default;
	AtomsMovedCR(AtomsMovedCR&&) noexcept = default;
	AtomsMovedCR& operator=(const AtomsMovedCR&) noexcept = default;
	AtomsMovedCR& operator=(AtomsMovedCR&&) noexcept = default;
	virtual ~AtomsMovedCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

	DirectX::XMFLOAT3 m_positionInitial;
	DirectX::XMFLOAT3 m_positionFinal;
	std::vector<size_t> m_indices;
};
}