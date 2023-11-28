#include "AtomMaterialCR.h"
#include "application/Application.h"


namespace seethe
{
void AtomMaterialCR::Undo(Application* app) noexcept
{
	app->SetMaterial(m_atomType, m_materialInitial);
}
void AtomMaterialCR::Redo(Application* app) noexcept
{
	app->SetMaterial(m_atomType, m_materialFinal);
}
}