#pragma once
#include "pch.h"
#include "utils/Log.h"
#include "utils/MathHelper.h"
#include "utils/Timer.h"


namespace seethe
{
class Camera
{
public:
	Camera() noexcept;
	Camera(const Camera&) noexcept = default;
	Camera(Camera&&) noexcept = default;
	Camera& operator=(const Camera&) noexcept = default;
	Camera& operator=(Camera&&) noexcept = default;
	~Camera() noexcept {}

	void Update(const Timer& timer) noexcept;

	// Get/Set world camera position.
	ND inline DirectX::XMVECTOR GetPosition() const noexcept { return DirectX::XMLoadFloat3(&m_position); }
	ND inline DirectX::XMFLOAT3 GetPosition3f() const noexcept { return m_position; }
	void SetPosition(float x, float y, float z) noexcept;
	void SetPosition(const DirectX::XMFLOAT3& v) noexcept;

	// Get camera basis vectors.
//	ND inline DirectX::XMVECTOR GetRight() const noexcept { return DirectX::XMLoadFloat3(&m_right); }
//	ND inline DirectX::XMFLOAT3 GetRight3f() const noexcept { return m_right; }
	ND inline DirectX::XMVECTOR GetUp() const noexcept { return DirectX::XMLoadFloat3(&m_up); }
	ND inline DirectX::XMFLOAT3 GetUp3f() const noexcept { return m_up; }
	ND inline DirectX::XMVECTOR GetLook() const noexcept { return DirectX::XMLoadFloat3(&m_lookAt); }
	ND inline DirectX::XMFLOAT3 GetLook3f() const noexcept { return m_lookAt; }

	// Get frustum properties.
	ND inline float GetNearZ() const noexcept { return m_nearZ; }
	ND inline float GetFarZ() const noexcept { return m_farZ; }
	ND inline float GetAspect() const noexcept { return m_aspect; }
	ND inline float GetFovY() const noexcept { return m_fovY; }
	ND inline float GetFovX() const noexcept
	{
		float halfWidth = 0.5f * GetNearWindowWidth();
		return 2.0f * atan(halfWidth / m_nearZ);
	}

	// Get near and far plane dimensions in view space coordinates.
	ND inline float GetNearWindowWidth() const noexcept { return m_aspect * m_nearWindowHeight; }
	ND inline float GetNearWindowHeight() const noexcept { return m_nearWindowHeight; }
	ND inline float GetFarWindowWidth() const noexcept { return m_aspect * m_farWindowHeight; }
	ND inline float GetFarWindowHeight() const noexcept { return m_farWindowHeight; }

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf) noexcept;

	// Define camera space via LookAt parameters.
//	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp) noexcept;
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) noexcept;

	// Get View/Proj matrices.
	ND inline DirectX::XMMATRIX GetView() const noexcept
	{
		ASSERT(!m_viewDirty, "View matrix is not up to date");
		return XMLoadFloat4x4(&m_view);
	}
	ND inline DirectX::XMMATRIX GetProj() const noexcept { return DirectX::XMLoadFloat4x4(&m_proj); }

	ND inline DirectX::XMFLOAT4X4 GetView4x4f() const noexcept
	{
		ASSERT(!m_viewDirty, "View matrix is not up to date");
		return m_view;
	}
	ND inline DirectX::XMFLOAT4X4 GetProj4x4f() const noexcept { return m_proj; }

//	// Strafe/Walk the camera a distance d.
//	void Strafe(float d) noexcept;
//	void Walk(float d) noexcept;
//
//	// Rotate the camera in place (meaning the camera does not change location, just what it is looking at)
//	void Pitch(float angle) noexcept;
//	void RotateY(float angle) noexcept;

	// Rotate the camera relative to the look at point (meaning the look at point stays the same, but the camera changes location)
	inline void RotateAroundLookAtPoint(float thetaX, float thetaY) noexcept { RotateAroundLookAtPointX(thetaX); RotateAroundLookAtPointY(thetaY); }
	void RotateAroundLookAtPointX(float thetaX) noexcept;
	void RotateAroundLookAtPointY(float thetaY) noexcept;
	void RotateAroundLookAtPointClockwise(float theta) noexcept;


	void ZoomInFixed(float fixedDistance) noexcept;
	void ZoomOutFixed(float fixedDistance) noexcept;
	void ZoomInPercent(float percent) noexcept;
	void ZoomOutPercent(float percent) noexcept;

	// Methods to initiate an animation (so the camera continually moves until it reaches a final destination)
	inline void StartAnimatedMove(float duration, const DirectX::XMFLOAT3& finalPosition) noexcept { StartAnimatedMove(duration, finalPosition, m_up, m_lookAt); }
	inline void StartAnimatedMove(float duration, const DirectX::XMFLOAT3& finalPosition, const DirectX::XMFLOAT3& finalUp) noexcept { StartAnimatedMove(duration, finalPosition, finalUp, m_lookAt); }
	void StartAnimatedMove(float duration, const DirectX::XMFLOAT3& finalPosition, const DirectX::XMFLOAT3& finalUp, const DirectX::XMFLOAT3& finalLookAt) noexcept;

	void StartConstantLeftRotation() noexcept;
	void StartConstantRightRotation() noexcept;
	void StartConstantUpRotation() noexcept;
	void StartConstantDownRotation() noexcept;
	void StartConstantClockwiseRotation() noexcept;
	void StartConstantCounterClockwiseRotation() noexcept;
	constexpr inline void StopConstantLeftRotation() noexcept { m_isInConstantRotationLeft = false; }
	constexpr inline void StopConstantRightRotation() noexcept { m_isInConstantRotationRight = false; }
	constexpr inline void StopConstantUpRotation() noexcept { m_isInConstantRotationUp = false; }
	constexpr inline void StopConstantDownRotation() noexcept { m_isInConstantRotationDown = false; }
	constexpr inline void StopConstantClockwiseRotation() noexcept { m_isInConstantRotationClockwise = false; }
	constexpr inline void StopConstantCounterClockwiseRotation() noexcept { m_isInConstantRotationCounterClockwise = false; }

	void ZoomInFixed(float fixedDistance, float duration) noexcept;
	void ZoomOutFixed(float fixedDistance, float duration) noexcept;
	void ZoomInPercent(float percent, float duration) noexcept;
	void ZoomOutPercent(float percent, float duration) noexcept;

	void CenterOnFace() noexcept;
	void Start90DegreeRotationLeft(float duration = 0.5f) noexcept;
	void Start90DegreeRotationRight(float duration = 0.5f) noexcept;
	void Start90DegreeRotationUp(float duration = 0.5f) noexcept;
	void Start90DegreeRotationDown(float duration = 0.5f) noexcept;
	void Start90DegreeRotationClockwise(float duration = 0.5f) noexcept;
	void Start90DegreeRotationCounterClockwise(float duration = 0.5f) noexcept;

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix() noexcept;

	ND constexpr inline bool IsInConstantRotation() const noexcept { return m_isInConstantRotationLeft || m_isInConstantRotationRight || m_isInConstantRotationUp || m_isInConstantRotationDown || m_isInConstantRotationClockwise || m_isInConstantRotationCounterClockwise; }

private:
	ND DirectX::XMFLOAT3 ZoomFixedImpl(float fixedDistance) const noexcept; 
	ND DirectX::XMFLOAT3 ZoomPercentImpl(float percent) const noexcept; 

	ND DirectX::XMFLOAT3 ComputePositionAfterLeftRightRotation(float theta) const noexcept;
	ND std::tuple<DirectX::XMFLOAT3, DirectX::XMFLOAT3> ComputePositionAndUpAfterUpDownRotation(float theta) const noexcept;
	ND DirectX::XMFLOAT3 ComputeUpAfterClockwiseRotation(float theta) const noexcept;

	ND DirectX::XMVECTOR RotateVector(DirectX::XMVECTOR vectorToRotate, DirectX::XMVECTOR axis, float theta) const noexcept;

	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
//	DirectX::XMFLOAT3 m_right = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 m_lookAt = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float m_nearZ = 0.0f;
	float m_farZ = 0.0f;
	float m_aspect = 0.0f;
	float m_fovY = 0.0f;
	float m_nearWindowHeight = 0.0f;
	float m_farWindowHeight = 0.0f;

	bool m_viewDirty = true;

	// Cache View/Proj matrices.
	DirectX::XMFLOAT4X4 m_view = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::Identity4x4();

	// Animated move variables
	bool m_performingAnimatedMove = false;
	DirectX::XMVECTOR m_targetPosition;
	DirectX::XMVECTOR m_targetUp;
	DirectX::XMVECTOR m_targetLook;
	DirectX::XMVECTOR m_initialPosition;
	DirectX::XMVECTOR m_initialUp;
	DirectX::XMVECTOR m_initialLook;
	float m_movementDuration = 0.0f;
	float m_movementStartTime = -1.0f;

	// Constant Rotation Variables
	bool m_isInConstantRotationLeft = false;
	bool m_isInConstantRotationRight = false;
	bool m_isInConstantRotationUp = false;
	bool m_isInConstantRotationDown = false;
	bool m_isInConstantRotationClockwise = false;
	bool m_isInConstantRotationCounterClockwise = false;

};
}