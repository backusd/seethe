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

//void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp) noexcept
//{
//	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
//	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
//	XMVECTOR U = XMVector3Cross(L, R);
//
//	XMStoreFloat3(&m_position, pos);
//	XMStoreFloat3(&m_lookAt, L);
//	XMStoreFloat3(&m_right, R);
//	XMStoreFloat3(&m_up, U);
//
//	m_viewDirty = true;
//}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up) noexcept
{
//	XMVECTOR P = XMLoadFloat3(&pos);
//	XMVECTOR T = XMLoadFloat3(&target);
//	XMVECTOR U = XMLoadFloat3(&up);
//
//	LookAt(P, T, U);

	m_position = pos;
	m_lookAt = target;
	m_up = up;

	m_viewDirty = true;
}


//void Camera::Strafe(float d) noexcept
//{
//	// m_position += d*m_right
//	XMVECTOR s = XMVectorReplicate(d);
//	XMVECTOR r = XMLoadFloat3(&m_right);
//	XMVECTOR p = XMLoadFloat3(&m_position);
//	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, r, p));
//
//	m_viewDirty = true;
//}
//
//void Camera::Walk(float d) noexcept
//{
//	// m_position += d*m_lookAt
//	XMVECTOR s = XMVectorReplicate(d);
//	XMVECTOR l = XMLoadFloat3(&m_lookAt);
//	XMVECTOR p = XMLoadFloat3(&m_position);
//	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, l, p));
//
//	m_viewDirty = true;
//}
//
//void Camera::Pitch(float angle) noexcept
//{
//	// Rotate up and look vector about the right vector.
//
//	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);
//
//	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
//	XMStoreFloat3(&m_lookAt, XMVector3TransformNormal(XMLoadFloat3(&m_lookAt), R));
//
//	m_viewDirty = true;
//}
//
//void Camera::RotateY(float angle) noexcept
//{
//	// Rotate the basis vectors about the world y-axis.
//
//	XMMATRIX R = XMMatrixRotationY(angle);
//
//	XMStoreFloat3(&m_right, XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
//	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
//	XMStoreFloat3(&m_lookAt, XMVector3TransformNormal(XMLoadFloat3(&m_lookAt), R));
//
//	m_viewDirty = true;
//}

void Camera::RotateAroundLookAtPointX(float thetaX) noexcept
{
	// Use Rodrigue's Rotation Formula
	//     See here: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
	//     v_rot : vector after rotation
	//     v     : vector before rotation
	//     theta : angle of rotation
	//     k     : unit vector representing axis of rotation
	//     v_rot = v*cos(theta) + (k x v)*sin(theta) + k*(k dot v)*(1-cos(theta))

	// NOTE: We subtract the LookAt vector and then add it back at the end. This way we can rotate around LookAt points other than the origin

	XMVECTOR l = XMLoadFloat3(&m_lookAt);
	XMVECTOR v = XMLoadFloat3(&m_position) - l;
	XMVECTOR k = XMVector3Normalize(XMLoadFloat3(&m_up));
	v = v * cos(thetaX) + XMVector3Cross(k, v) * sin(thetaX) + k * XMVector3Dot(k, v) * (1 - cos(thetaX));
	v += l;
	XMStoreFloat3(&m_position, v);

	m_viewDirty = true;
}
void Camera::RotateAroundLookAtPointY(float thetaY) noexcept
{
	// Use Rodrigue's Rotation Formula
	//     See here: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
	//     v_rot : vector after rotation
	//     v     : vector before rotation
	//     theta : angle of rotation
	//     k     : unit vector representing axis of rotation
	//     v_rot = v*cos(theta) + (k x v)*sin(theta) + k*(k dot v)*(1-cos(theta))
	
	// NOTE: We subtract the LookAt vector and then add it back at the end. This way we can rotate around LookAt points other than the origin
	
	// The axis of rotation vector for up/down rotation will be the cross product 
	// between the eye-vector and the up-vector (must make it a unit vector)
	XMVECTOR l = XMLoadFloat3(&m_lookAt);
	XMVECTOR v = XMLoadFloat3(&m_position) - l; 
	XMVECTOR u = XMLoadFloat3(&m_up);
	XMVECTOR k = XMVector3Normalize(XMVector3Cross(v, u));
	v = v * cos(thetaY) + XMVector3Cross(k, v) * sin(thetaY) + k * XMVector3Dot(k, v) * (1 - cos(thetaY)); 
	v += l;
	XMStoreFloat3(&m_position, v); 

	// Now update the new up-vector should be the cross product between the k-vector and the new position vector
	u = XMVector3Normalize(XMVector3Cross(k, v));
	XMStoreFloat3(&m_up, u);

	m_viewDirty = true;
}

void Camera::UpdateViewMatrix() noexcept
{
	if (m_viewDirty)
	{
//		XMVECTOR R = XMLoadFloat3(&m_right);
		XMVECTOR U = XMLoadFloat3(&m_up);
		XMVECTOR L = XMLoadFloat3(&m_lookAt);
		XMVECTOR P = XMLoadFloat3(&m_position);

		XMMATRIX view = XMMatrixLookAtLH(P, L, U);
		XMStoreFloat4x4(&m_view, view);

		m_viewDirty = false;

//		// Keep camera's axes orthogonal to each other and of unit length.
//		L = XMVector3Normalize(L);
//		U = XMVector3Normalize(XMVector3Cross(L, R));
//
//		// U, L already ortho-normal, so no need to normalize cross product.
//		R = XMVector3Cross(U, L);
//
//		// Fill in the view matrix entries.
//		float x = -XMVectorGetX(XMVector3Dot(P, R));
//		float y = -XMVectorGetX(XMVector3Dot(P, U));
//		float z = -XMVectorGetX(XMVector3Dot(P, L));
//
//		XMStoreFloat3(&m_right, R);
//		XMStoreFloat3(&m_up, U);
//		XMStoreFloat3(&m_lookAt, L);
//
//		m_view(0, 0) = m_right.x;
//		m_view(1, 0) = m_right.y;
//		m_view(2, 0) = m_right.z;
//		m_view(3, 0) = x;
//
//		m_view(0, 1) = m_up.x;
//		m_view(1, 1) = m_up.y;
//		m_view(2, 1) = m_up.z;
//		m_view(3, 1) = y;
//
//		m_view(0, 2) = m_lookAt.x;
//		m_view(1, 2) = m_lookAt.y;
//		m_view(2, 2) = m_lookAt.z;
//		m_view(3, 2) = z;
//
//		m_view(0, 3) = 0.0f;
//		m_view(1, 3) = 0.0f;
//		m_view(2, 3) = 0.0f;
//		m_view(3, 3) = 1.0f;
//
//		m_viewDirty = false;
	}
}

void Camera::Update(const Timer& timer) noexcept
{
	if (m_performingAnimatedMove)
	{
		float totalTime = timer.TotalTime();

		if (m_movementStartTime < 0)
			m_movementStartTime = totalTime - timer.DeltaTime();

		// Compute the ratio of elapsed time / allowed time to complete
		float timeRatio = (totalTime - m_movementStartTime) / m_movementDuration;

		// if the current time is beyond the given duration, assign all vectors to their target values
		if (timeRatio >= 1.0f)
		{
			m_performingAnimatedMove = false;
			XMStoreFloat3(&m_position, m_targetPosition);
			XMStoreFloat3(&m_up, m_targetUp);
			XMStoreFloat3(&m_lookAt, m_targetLook);
		}
		else
		{
			// Using the time ratio, compute the intermediate position/up/look vector values
			XMVECTOR pos = m_initialPosition + ((m_targetPosition - m_initialPosition) * timeRatio);
			XMStoreFloat3(&m_position, pos);

			XMVECTOR up = m_initialUp + ((m_targetUp - m_initialUp) * timeRatio);
			XMStoreFloat3(&m_up, up);

			XMVECTOR look = m_initialLook + ((m_targetLook - m_initialLook) * timeRatio);
			XMStoreFloat3(&m_lookAt, look);
		}

		m_viewDirty = true;
	}

	// Make sure to try to update the view matrix in case anything changed
	UpdateViewMatrix();
}

void Camera::StartAnimatedMove(float duration, const DirectX::XMFLOAT3& finalPosition, const DirectX::XMFLOAT3& finalUp, const DirectX::XMFLOAT3& finalLookAt) noexcept
{
	ASSERT(duration > 0.0, "Animated duration must not be negative");
	m_performingAnimatedMove = true;
	m_targetPosition = XMLoadFloat3(&finalPosition);
	m_targetUp = XMLoadFloat3(&finalUp);
	m_targetLook = XMLoadFloat3(&finalLookAt);
	m_initialPosition = XMLoadFloat3(&m_position);
	m_initialUp = XMLoadFloat3(&m_up);
	m_initialLook = XMLoadFloat3(&m_lookAt);
	m_movementDuration = duration;
	m_movementStartTime = -1.0;
	m_viewDirty = true;
}

void Camera::ZoomInFixed(float fixedDistance) noexcept
{
	ASSERT(fixedDistance > 0.0f, "When zooming, the fixedDistance should always be > 0");

	// Keep the fixedDistance positive. ZoomFixedImpl assumes a positive fixedDistance means zooming in and negative means zooming out
	m_position = ZoomFixedImpl(fixedDistance);
	m_viewDirty = true;
}
void Camera::ZoomOutFixed(float fixedDistance) noexcept
{
	ASSERT(fixedDistance > 0.0f, "When zooming, the fixedDistance should always be > 0");

	// Make the fixedDistance negative. ZoomFixedImpl assumes a positive fixedDistance means zooming in and negative means zooming out
	m_position = ZoomFixedImpl(-fixedDistance);
	m_viewDirty = true;
}
void Camera::ZoomInPercent(float percent) noexcept
{
	ASSERT(percent > 0.0f, "When zooming by percent, the percent value should always be > 0");
	ASSERT(percent < 1.0f, "When zooming in by percent, the percent value should always be < 1");

	// Keep the percent positive. ZoomPercentImpl assumes a positive percent means zooming in and negative means zooming out
	m_position = ZoomPercentImpl(percent);
	m_viewDirty = true;
}
void Camera::ZoomOutPercent(float percent) noexcept
{
	ASSERT(percent > 0.0f, "When zooming by percent, the percent value should always be > 0");
	// NOTE: There is no restriction that Zooming Out can't have the percent be > 1 (for example, 200% makes perfect sense for zooming out)

	// Make the percent negative. ZoomPercentImpl assumes a positive percent means zooming in and negative means zooming out
	m_position = ZoomPercentImpl(-percent);
	m_viewDirty = true;
}

XMFLOAT3 Camera::ZoomFixedImpl(float fixedDistance) const noexcept
{
	XMFLOAT3 newPosition;

	XMVECTOR position = XMLoadFloat3(&m_position); 
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt); 
	XMVECTOR directionToMove = lookAt - position;

	// If the zoom distance is > 0, then we are zooming in and therefore need to make sure we have room and don't zoom beyond the lookAt point
	if (fixedDistance > 0)
	{
		// Note, we add a small margin so that if the requested fixed distance is greater than the available space, 
		// we don't max out the fixed distance and instead set it just shy of the maximum. This makes it so that the
		// resulting position vector and lookAt vector are never equal
		fixedDistance = std::min(fixedDistance, XMVectorGetX(XMVector3Length(directionToMove)) - 0.05f);
	}

	// First, make the direction to move a unit vector, and then scale it to the distance we want to move
	directionToMove = XMVector3Normalize(directionToMove) * fixedDistance;

	// Second, add this result to the position vector which should give us the final position we want to be at
	position += directionToMove;

	XMStoreFloat3(&newPosition, position);
	return newPosition;
}
XMFLOAT3 Camera::ZoomPercentImpl(float percent) const noexcept
{
	XMFLOAT3 newPosition;

	XMVECTOR position = XMLoadFloat3(&m_position);
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt);
	XMVECTOR directionToMove = lookAt - position;

	const float zoomDistance = XMVectorGetX(XMVector3Length(directionToMove)) * percent;

	// First, make the direction to move a unit vector, and then scale it to the distance we want to move
	directionToMove = XMVector3Normalize(directionToMove) * zoomDistance; 

	// Second, add this result to the position vector which should give us the final position we want to be at
	position += directionToMove;

	XMStoreFloat3(&newPosition, position);
	return newPosition;
}

void Camera::ZoomInFixed(float fixedDistance, float duration) noexcept
{
	ASSERT(fixedDistance > 0.0f, "When zooming, the fixedDistance should always be > 0");
	ASSERT(duration > 0.0f, "When zooming, the duration should always be > 0");

	// Keep the fixedDistance positive. ZoomFixedImpl assumes a positive fixedDistance means zooming in and negative means zooming out
	StartAnimatedMove(duration, ZoomFixedImpl(fixedDistance));
}
void Camera::ZoomOutFixed(float fixedDistance, float duration) noexcept
{
	ASSERT(fixedDistance > 0.0f, "When zooming, the fixedDistance should always be > 0"); 
	ASSERT(duration > 0.0f, "When zooming, the duration should always be > 0"); 
	
	// Make the fixedDistance negative. ZoomFixedImpl assumes a positive fixedDistance means zooming in and negative means zooming out
	StartAnimatedMove(duration, ZoomFixedImpl(-fixedDistance));
}
void Camera::ZoomInPercent(float percent, float duration) noexcept
{
	ASSERT(percent > 0.0f, "When zooming by percent, the percent value should always be > 0");
	ASSERT(percent < 1.0f, "When zooming in by percent, the percent value should always be < 1");
	ASSERT(duration > 0.0f, "When zooming, the duration should always be > 0");

	// Keep the percent positive. ZoomPercentImpl assumes a positive percent means zooming in and negative means zooming out
	StartAnimatedMove(duration, ZoomPercentImpl(percent));
}
void Camera::ZoomOutPercent(float percent, float duration) noexcept
{
	ASSERT(percent > 0.0f, "When zooming by percent, the percent value should always be > 0");
	ASSERT(duration > 0.0f, "When zooming, the duration should always be > 0");

	// NOTE: There is no restriction that Zooming Out can't have the percent be > 1 (for example, 200% makes perfect sense for zooming out)

	// Make the percent negative. ZoomPercentImpl assumes a positive percent means zooming in and negative means zooming out
	StartAnimatedMove(duration, ZoomPercentImpl(-percent));
}

}