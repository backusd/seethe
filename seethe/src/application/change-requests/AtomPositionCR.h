#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
	class AtomPositionCR : public ChangeRequest
	{
	public:
		AtomPositionCR(const DirectX::XMFLOAT3& initialPosition, const DirectX::XMFLOAT3& finalPosition, unsigned int atomUUID) noexcept :
			m_positionInitial(initialPosition),
			m_positionFinal(finalPosition),
			m_atomUUID(atomUUID)
		{}
		AtomPositionCR(const AtomPositionCR&) noexcept = default;
		AtomPositionCR(AtomPositionCR&&) noexcept = default;
		AtomPositionCR& operator=(const AtomPositionCR&) noexcept = default;
		AtomPositionCR& operator=(AtomPositionCR&&) noexcept = default;
		virtual ~AtomPositionCR() noexcept = default;

		void Undo(Application* app) noexcept override;
		void Redo(Application* app) noexcept override;

		DirectX::XMFLOAT3 m_positionInitial;
		DirectX::XMFLOAT3 m_positionFinal;
		unsigned int m_atomUUID;
	};
}