#include "Camera.h"

using namespace DirectX;

namespace seethe
{
Camera::Camera() noexcept
{
	SetLens(0.25f * MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
}

void Camera::SetPosition(float x, float y, float z) noexcept
{
	m_position = XMFLOAT3(x, y, z);
	m_viewDirty = true;
}
void Camera::SetPosition(const XMFLOAT3& v) noexcept
{
	m_position = v;
	m_viewDirty = true;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf) noexcept
{
	// cache properties
	m_fovY = fovY;
	m_aspect = aspect;
	m_nearZ = zn;
	m_farZ = zf;

	m_nearWindowHeight = 2.0f * m_nearZ * tanf(0.5f * m_fovY);
	m_farWindowHeight = 2.0f * m_farZ * tanf(0.5f * m_fovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_proj, P);
}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp) noexcept
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_position, pos);
	XMStoreFloat3(&m_look, L);
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);

	m_viewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up) noexcept
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	m_viewDirty = true;
}


void Camera::Strafe(float d) noexcept
{
	// m_position += d*m_right
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&m_right);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, r, p));

	m_viewDirty = true;
}

void Camera::Walk(float d) noexcept
{
	// m_position += d*m_look
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&m_look);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, l, p));

	m_viewDirty = true;
}

void Camera::Pitch(float angle) noexcept
{
	// Rotate up and look vector about the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);

	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));

	m_viewDirty = true;
}

void Camera::RotateY(float angle) noexcept
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));

	m_viewDirty = true;
}

void Camera::UpdateViewMatrix() noexcept
{
	if (m_viewDirty)
	{
		XMVECTOR R = XMLoadFloat3(&m_right);
		XMVECTOR U = XMLoadFloat3(&m_up);
		XMVECTOR L = XMLoadFloat3(&m_look);
		XMVECTOR P = XMLoadFloat3(&m_position);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&m_right, R);
		XMStoreFloat3(&m_up, U);
		XMStoreFloat3(&m_look, L);

		m_view(0, 0) = m_right.x;
		m_view(1, 0) = m_right.y;
		m_view(2, 0) = m_right.z;
		m_view(3, 0) = x;

		m_view(0, 1) = m_up.x;
		m_view(1, 1) = m_up.y;
		m_view(2, 1) = m_up.z;
		m_view(3, 1) = y;

		m_view(0, 2) = m_look.x;
		m_view(1, 2) = m_look.y;
		m_view(2, 2) = m_look.z;
		m_view(3, 2) = z;

		m_view(0, 3) = 0.0f;
		m_view(1, 3) = 0.0f;
		m_view(2, 3) = 0.0f;
		m_view(3, 3) = 1.0f;

		m_viewDirty = false;
	}
}
}