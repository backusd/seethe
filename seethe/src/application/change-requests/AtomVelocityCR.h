#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class AtomVelocityCR : public ChangeRequest
{
public:
	AtomVelocityCR(const DirectX::XMFLOAT3& initialVelocity, const DirectX::XMFLOAT3 & finalVelocity, size_t atomUUID) noexcept :
		m_velocityInitial(initialVelocity),
		m_velocityFinal(finalVelocity),
		m_atomUUID(atomUUID)
	{}
	AtomVelocityCR(const AtomVelocityCR&) noexcept = default;
	AtomVelocityCR(AtomVelocityCR&&) noexcept = default;
	AtomVelocityCR& operator=(const AtomVelocityCR&) noexcept = default;
	AtomVelocityCR& operator=(AtomVelocityCR&&) noexcept = default;
	virtual ~AtomVelocityCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

	DirectX::XMFLOAT3 m_velocityInitial;
	DirectX::XMFLOAT3 m_velocityFinal;
	size_t m_atomUUID;
};
}