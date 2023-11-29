#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"

namespace seethe
{
class AddAtomCR : public ChangeRequest
{
public:
	AddAtomCR(size_t uuid) noexcept : m_uuid(uuid) {}
	AddAtomCR(const AddAtomCR&) noexcept = default;
	AddAtomCR(AddAtomCR&&) noexcept = default;
	AddAtomCR& operator=(const AddAtomCR&) noexcept = default;
	AddAtomCR& operator=(AddAtomCR&&) noexcept = default;
	virtual ~AddAtomCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

private:
	size_t				m_uuid;

	// These values can be defaulted because we will need to updated them anyways when
	// the undo event is triggered
	DirectX::XMFLOAT3	m_position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3	m_velocity = { 0.0f, 0.0f, 0.0f };
	AtomType			m_type = AtomType::HYDROGEN;
};
}