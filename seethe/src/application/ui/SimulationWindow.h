#pragma once
#include "pch.h"
#include "rendering/Renderer.h"
#include "application/rendering/Light.h"
#include "application/rendering/Material.h"
#include "simulation/Simulation.h"
#include "utils/Timer.h"
#include "Enums.h"


struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = seethe::MathHelper::Identity4x4();
};

struct InstanceData
{
	DirectX::XMFLOAT4X4 World;
	std::uint32_t MaterialIndex;
	std::uint32_t Pad0;
	std::uint32_t Pad1;
	std::uint32_t Pad2;
};

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;

	// This method is required because we impose the HAS_POSITION concept on MeshGroupT so that we can compute the bounding box
	ND inline DirectX::XMFLOAT3 Position() const noexcept { return Pos; }
};
struct SolidColorVertex
{
	DirectX::XMFLOAT4 Pos;

	// This method is required because we impose the HAS_POSITION concept on MeshGroupT so that we can compute the bounding box
	ND inline DirectX::XMFLOAT3 Position() const noexcept { return { Pos.x, Pos.y, Pos.z }; }
};




struct PassConstants
{
	DirectX::XMFLOAT4X4 View = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = seethe::MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

//	seethe::SceneLighting Lighting = {};

//	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };
//
//	// Indices [0, NUM_DIR_LIGHTS) are directional lights;
//	// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
//	// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
//	// are spot lights for a maximum of MaxLights per object.
//	seethe::Light Lights[MaxLights];
};

namespace seethe
{
class Application;

class SimulationWindow
{
public:
	SimulationWindow(Application& application,
		std::shared_ptr<DeviceResources> deviceResources, 
		Simulation& simulation, 
		std::vector<Material>& materials,
		SceneLighting& lighting,
		float top, float left, float height, float width);

	void Update(const Timer& timer, int frameIndex);
	inline void Render(int frameIndex) { m_renderer->Render(m_simulation, frameIndex); }
	inline void SetWindow(float top, float left, float height, float width) noexcept
	{
		m_viewport = { left, top, width, height, 0.0f, 1.0f };
		m_scissorRect = { static_cast<long>(left), static_cast<long>(top), static_cast<long>(left + width), static_cast<long>(top + height) };
	}

	ND bool OnLButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseLButtonDown, x, y, [this]() { HandleLButtonDown(); }); }
	ND bool OnLButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseLButtonDown, x, y, [this]() { HandleLButtonUp(); }); }
	ND bool OnLButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleLButtonDoubleClick(); }); }
	ND bool OnMButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseMButtonDown, x, y, [this]() { HandleMButtonDown(); }); }
	ND bool OnMButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseMButtonDown, x, y, [this]() { HandleMButtonUp(); }); }
	ND bool OnMButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleMButtonDoubleClick(); }); }
	ND bool OnRButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseRButtonDown, x, y, [this]() { HandleRButtonDown(); }); }
	ND bool OnRButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseRButtonDown, x, y, [this]() { HandleRButtonUp(); }); }
	ND bool OnRButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleRButtonDoubleClick(); }); }
	ND bool OnX1ButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseX1ButtonDown, x, y, [this]() { HandleX1ButtonDown(); }); }
	ND bool OnX1ButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseX1ButtonDown, x, y, [this]() { HandleX1ButtonUp(); }); }
	ND bool OnX1ButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleX1ButtonDoubleClick(); }); }
	ND bool OnX2ButtonDown(float x, float y) noexcept { return OnButtonDownImpl(m_mouseX2ButtonDown, x, y, [this]() { HandleX2ButtonDown(); }); }
	ND bool OnX2ButtonUp(float x, float y) noexcept { return OnButtonUpImpl(m_mouseX2ButtonDown, x, y, [this]() { HandleX2ButtonUp(); }); }
	ND bool OnX2ButtonDoubleClick(float x, float y) noexcept { return OnButtonDoubleClickImpl(x, y, [this]() { HandleX2ButtonDoubleClick(); }); }
	ND bool OnMouseMove(float x, float y) noexcept;
	ND bool OnMouseWheelVertical(int wheelDelta) noexcept;
	ND bool OnMouseWheelHorizontal(int wheelDelta) noexcept;

	ND bool OnKeyDown(unsigned int virtualKeyCode) noexcept;
	ND bool OnKeyUp(unsigned int virtualKeyCode) noexcept;
	ND bool OnChar(char c) noexcept;


	ND constexpr inline bool ContainsPoint(float x, float y) const noexcept { return m_viewport.TopLeftX <= x && m_viewport.TopLeftY <= y && m_viewport.TopLeftX + m_viewport.Width >= x && m_viewport.TopLeftY + m_viewport.Height >= y; }
	ND constexpr inline bool Dragging() const noexcept { return m_mouseLButtonDown || m_mouseMButtonDown || m_mouseRButtonDown || m_mouseX1ButtonDown || m_mouseX2ButtonDown; }

	constexpr inline void SetAllowMouseToResizeBoxDimensions(bool allow) noexcept
	{ 
		m_allowMouseToResizeBoxDimensions = allow; 
		ClearMouseHoverWallState();
		ClearMouseDraggingWallState();

		if (!allow)
			SetBoxWallResizeRenderEffectsActive(false);
	}

	void StartSelectionMovement(MovementDirection direction = MovementDirection::X) noexcept;
	void EndSelectionMovement() noexcept;

private:
	void RegisterEventHandlers() noexcept;

	void OnBoxSizeChanged() noexcept;
	void OnBoxFaceHighlightChanged() noexcept;
	void OnSelectedAtomsChanged() noexcept;
	void OnAtomsAdded() noexcept;
	void OnAtomsRemoved() noexcept;
	void OnSimulationPlay() noexcept;
	void OnSimulationPause() noexcept;

	bool OnButtonDownImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler);
	bool OnButtonUpImpl(bool& buttonFlag, float x, float y, std::function<void()>&& handler);
	bool OnButtonDoubleClickImpl(float x, float y, std::function<void()>&& handler);

	void HandleLButtonDown() noexcept;
	void HandleLButtonUp() noexcept;
	void HandleLButtonDoubleClick() noexcept;
	void HandleMButtonDown() noexcept;
	void HandleMButtonUp() noexcept;
	void HandleMButtonDoubleClick() noexcept;
	void HandleRButtonDown() noexcept;
	void HandleRButtonUp() noexcept;
	void HandleRButtonDoubleClick() noexcept;
	void HandleX1ButtonDown() noexcept;
	void HandleX1ButtonUp() noexcept;
	void HandleX1ButtonDoubleClick() noexcept;
	void HandleX2ButtonDown() noexcept;
	void HandleX2ButtonUp() noexcept;
	void HandleX2ButtonDoubleClick() noexcept;
	void HandleMouseMove(float x, float y) noexcept;
	void HandleMouseWheelVertical(int wheelDelta) noexcept;
	void HandleMouseWheelHorizontal(int wheelDelta) noexcept;
	void HandleKeyDown(unsigned int virtualKeyCode) noexcept;
	void HandleKeyUp(unsigned int virtualKeyCode) noexcept;
	void HandleChar(char c) noexcept;

	ND constexpr inline bool KeyboardKeyIsPressed() const noexcept 
	{ 
		return m_arrowLeftIsPressed || m_arrowRightIsPressed || m_arrowUpIsPressed || m_arrowDownIsPressed ||
			   m_keyAIsPressed || m_keyDIsPressed || m_keyEIsPressed || m_keyQIsPressed || m_keySIsPressed || m_keyWIsPressed;
	}
	ND constexpr inline bool MouseIsInViewport(float x, float y) const noexcept
	{
		return m_viewport.TopLeftX <= x && m_viewport.TopLeftX + m_viewport.Width >= x &&
			m_viewport.TopLeftY <= y && m_viewport.TopLeftY + m_viewport.Height >= y;
	}

	void InitializeRenderPasses();

	std::optional<size_t> PickAtom(float x, float y);
	void PickBoxWalls(float x, float y);

	constexpr inline void ClearMouseHoverWallState() noexcept
	{
		m_mouseHoveringBoxWallPosX = false;
		m_mouseHoveringBoxWallPosY = false;
		m_mouseHoveringBoxWallPosZ = false;
		m_mouseHoveringBoxWallNegX = false;
		m_mouseHoveringBoxWallNegY = false;
		m_mouseHoveringBoxWallNegZ = false;
		m_mouseDraggingBoxJustStarted = false;
	}
	constexpr inline void ClearMouseDraggingWallState() noexcept
	{
		m_mouseDraggingBoxWallPosX = false;
		m_mouseDraggingBoxWallPosY = false;
		m_mouseDraggingBoxWallPosZ = false;
		m_mouseDraggingBoxWallNegX = false;
		m_mouseDraggingBoxWallNegY = false;
		m_mouseDraggingBoxWallNegZ = false;
	}
	ND inline bool MouseIsDraggingWall() const noexcept
	{
		return m_mouseDraggingBoxWallPosX || m_mouseDraggingBoxWallPosY || m_mouseDraggingBoxWallPosZ ||
			m_mouseDraggingBoxWallNegX || m_mouseDraggingBoxWallNegY || m_mouseDraggingBoxWallNegZ;
	}
	ND inline bool MouseIsHoveringWall() const noexcept
	{
		return m_mouseHoveringBoxWallPosX || m_mouseHoveringBoxWallPosY || m_mouseHoveringBoxWallPosZ ||
			m_mouseHoveringBoxWallNegX || m_mouseHoveringBoxWallNegY || m_mouseHoveringBoxWallNegZ;
	}
	inline void SetBoxWallResizeRenderEffectsActive(bool active) noexcept
	{
		auto& pass1Layers = m_renderer->GetRenderPass(0).GetRenderPassLayers();

		// Box Wall transparency layer
		pass1Layers[3].SetActive(active);
		pass1Layers[3].GetRenderItems()[0].SetInstanceCount(2); // 2 instances because 2 walls

		// Arrow Render Item
		pass1Layers[0].GetRenderItems()[1].SetActive(active);
	}

	inline void SelectionMovementDirectionChanged() noexcept { m_oneTimeUpdateFns.push_back([this]() { SelectionMovementDirectionChangedImpl(); }); }
	void SelectionMovementDirectionChangedImpl() noexcept;
	inline void SelectionMovementDragPlaneChanged() noexcept { m_oneTimeUpdateFns.push_back([this]() { SelectionMovementDragPlaneChangedImpl(); }); }
	void SelectionMovementDragPlaneChangedImpl() noexcept;
	void DragSelectedAtoms(float x, float y) noexcept;

	std::vector<std::function<void()>> m_oneTimeUpdateFns;

	std::shared_ptr<DeviceResources> m_deviceResources;
	std::unique_ptr<Renderer> m_renderer;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Simulation& m_simulation;
	Application& m_application;
	SceneLighting& m_lighting;

	std::vector<Material>& m_atomMaterials;

	// Shaders
	std::unique_ptr<Shader> m_phongVSInstanced = nullptr;
	std::unique_ptr<Shader> m_phongPSInstanced = nullptr;
	std::unique_ptr<Shader> m_solidVS = nullptr;
	std::unique_ptr<Shader> m_solidPS = nullptr;

	// Input Layouts
	std::unique_ptr<InputLayout> m_inputLayoutInstanced = nullptr;
	std::unique_ptr<InputLayout> m_solidInputLayout = nullptr;

	// Mesh Groups
	std::shared_ptr<MeshGroup<Vertex>> m_sphereMeshGroup = nullptr;

	// Instance Data
	std::vector<InstanceData> m_instanceData;
	std::vector<InstanceData> m_selectedAtomsInstanceData;
	std::vector<InstanceData> m_selectedAtomsInstanceOutlineData;

	// Constant Buffers - Mapped
	std::unique_ptr<ConstantBufferMapped<InstanceData>>		m_instanceConstantBuffer;
	std::unique_ptr<ConstantBufferMapped<InstanceData>>		m_selectedAtomInstanceConstantBuffer;
	std::unique_ptr<ConstantBufferMapped<InstanceData>>		m_selectedAtomInstanceOutlineConstantBuffer;
	std::unique_ptr<ConstantBufferMapped<PassConstants>>	m_passConstantsBuffer;

	// Constant Buffers - Static
	std::unique_ptr<ConstantBufferStatic<SceneLighting>>	m_lightingConstantBuffer;
	std::unique_ptr<ConstantBufferStatic<Material>>			m_materialsConstantBuffer;
	std::unique_ptr<ConstantBufferStatic<InstanceData>>		m_boxConstantBuffer;
	std::unique_ptr<ConstantBufferStatic<InstanceData>>		m_boxFaceConstantBuffer;
	std::unique_ptr<ConstantBufferStatic<InstanceData>>		m_arrowConstantBuffer;
	std::unique_ptr<ConstantBufferStatic<InstanceData>>		m_axisCylinderConstantBuffer;





	const DirectX::BoundingBox m_boundingBoxPosX = { { 1.0f,  0.0f,  0.0f}, { 0.0f, 1.0f, 1.0f } };
	const DirectX::BoundingBox m_boundingBoxNegX = { {-1.0f,  0.0f,  0.0f}, { 0.0f, 1.0f, 1.0f } };
	const DirectX::BoundingBox m_boundingBoxPosY = { { 0.0f,  1.0f,  0.0f}, { 1.0f, 0.05f, 1.0f } };
	const DirectX::BoundingBox m_boundingBoxNegY = { { 0.0f, -1.0f,  0.0f}, { 1.0f, 0.05f, 1.0f } };
	const DirectX::BoundingBox m_boundingBoxPosZ = { { 0.0f,  0.0f,  1.0f}, { 1.0f, 1.0f, 0.05f } };
	const DirectX::BoundingBox m_boundingBoxNegZ = { { 0.0f,  0.0f, -1.0f}, { 1.0f, 1.0f, 0.05f } };


	// Mouse Tracking
	bool m_mouseLButtonDown = false;
	bool m_mouseMButtonDown = false;
	bool m_mouseRButtonDown = false;
	bool m_mouseX1ButtonDown = false;
	bool m_mouseX2ButtonDown = false;
	DirectX::XMFLOAT2 m_mouseLastPos = DirectX::XMFLOAT2();

	// Keyboard Tracking
	bool m_arrowLeftIsPressed = false;
	bool m_arrowRightIsPressed = false;
	bool m_arrowUpIsPressed = false;
	bool m_arrowDownIsPressed = false;
	bool m_shiftIsPressed = false;
	bool m_keyWIsPressed = false;
	bool m_keyAIsPressed = false;
	bool m_keySIsPressed = false;
	bool m_keyDIsPressed = false;
	bool m_keyQIsPressed = false;
	bool m_keyEIsPressed = false;

	// State Info
	bool m_allowMouseToResizeBoxDimensions = false;
	bool m_mouseHoveringBoxWallPosX = false;
	bool m_mouseHoveringBoxWallPosY = false;
	bool m_mouseHoveringBoxWallPosZ = false;
	bool m_mouseHoveringBoxWallNegX = false;
	bool m_mouseHoveringBoxWallNegY = false;
	bool m_mouseHoveringBoxWallNegZ = false;
	bool m_mouseDraggingBoxWallPosX = false;
	bool m_mouseDraggingBoxWallPosY = false;
	bool m_mouseDraggingBoxWallPosZ = false;
	bool m_mouseDraggingBoxWallNegX = false;
	bool m_mouseDraggingBoxWallNegY = false;
	bool m_mouseDraggingBoxWallNegZ = false;
	float m_mousePrevX = 0.0f;
	float m_mousePrevY = 0.0f;
	bool m_mouseDraggingBoxJustStarted = false;
	bool m_mouseDraggingBoxRightIsLarger = false;
	bool m_mouseDraggingBoxUpIsLarger = false;
	DirectX::XMFLOAT3 m_boxDimensionsInitial = {};
	bool m_forceSidesToBeEqualInitial = true;

	// State when atom(s) is being moved
	bool m_selectionBeingMovedStateIsActive = false;
	bool m_selectionIsBeingDragged = false;
	MovementDirection m_movementDirection = MovementDirection::X;
	std::optional<size_t> m_atomHoveredOverIndex = std::nullopt;
	DirectX::XMFLOAT3 selectionCenterAtStartOfDrag = { 0.0f, 0.0f, 0.0f };
};
}