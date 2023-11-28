#pragma once
#include "pch.h"
#include "ChangeRequest.h"
#include "simulation/Simulation.h"
#include "application/rendering/Material.h"

namespace seethe
{
class AtomMaterialCR : public ChangeRequest
{
public:
	AtomMaterialCR(const Material& initialMaterial, const Material& finalMaterial, AtomType atomType) noexcept :
		m_materialInitial(initialMaterial),
		m_materialFinal(finalMaterial),
		m_atomType(atomType)
	{}
	AtomMaterialCR(const AtomMaterialCR&) noexcept = default;
	AtomMaterialCR(AtomMaterialCR&&) noexcept = default;
	AtomMaterialCR& operator=(const AtomMaterialCR&) noexcept = default;
	AtomMaterialCR& operator=(AtomMaterialCR&&) noexcept = default;
	virtual ~AtomMaterialCR() noexcept = default;

	void Undo(Application* app) noexcept override;
	void Redo(Application* app) noexcept override;

	Material m_materialInitial;
	Material m_materialFinal;
	AtomType m_atomType;
};
}