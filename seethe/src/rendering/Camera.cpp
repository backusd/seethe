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
	m_position = ComputePositionAfterLeftRightRotation(thetaX);
	m_viewDirty = true;
}
void Camera::RotateAroundLookAtPointY(float thetaY) noexcept
{
	auto [position, up] = ComputePositionAndUpAfterUpDownRotation(thetaY);
	m_position = position;
	m_up = up;

	m_viewDirty = true;
}
void Camera::RotateAroundLookAtPointClockwise(float theta) noexcept
{
	m_up = ComputeUpAfterClockwiseRotation(theta);
	m_viewDirty = true;
}

XMVECTOR Camera::RotateVector(XMVECTOR v, XMVECTOR k, float theta) const noexcept
{
	// Use Rodrigue's Rotation Formula
	//     See here: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
	//     v_rot : vector after rotation
	//     v     : vector before rotation
	//     theta : angle of rotation
	//     k     : unit vector representing axis of rotation
	//     v_rot = v*cos(theta) + (k x v)*sin(theta) + k*(k dot v)*(1-cos(theta))
	//return v * cos(theta) + XMVector3Cross(k, v) * sin(theta) + k * XMVector3Dot(k, v) * (1 - cos(theta));

	return XMVector3Rotate(v, XMQuaternionRotationAxis(k, theta));	
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

			// If all we did was implement the code above, we would be fine. However, when the lookAt point stays the same AND
			// the total distance to the look at point doesn't change, it would be ideal if the path the camera took from start 
			// to finish kept the same distance from the lookAt point, instead of getting closer and then subsequently further. 
			// Put another way, it would be better for the camera to move along an arc from start to finish rather than move in 
			// a straight line. Therefore, when the lookAt point does not change, we make the following adjustment to the camera 
			// movement
			bool lookAtStaysTheSame = XMVectorGetX(XMVector3Length(m_targetLook - m_initialLook)) < 0.05f;
			bool distanceToLookAtStaysTheSame = std::abs(XMVectorGetX(XMVector3Length(m_initialPosition - m_initialLook)) - XMVectorGetX(XMVector3Length(m_targetPosition - m_initialLook))) < 0.05f;
			if (lookAtStaysTheSame && distanceToLookAtStaysTheSame)
			{
				float distanceToMaintain = XMVectorGetX(XMVector3Length(m_initialPosition - m_initialLook));
				XMVECTOR direction = pos - look;
				float currentDistance = XMVectorGetX(XMVector3Length(direction));
				pos += XMVector3Normalize(direction) * (distanceToMaintain - currentDistance);
				XMStoreFloat3(&m_position, pos);
			}
		}

		m_viewDirty = true;
	}
	else if (IsInConstantRotation())
	{
		// Compute the rotation
		const float radiansPerSecond = 1.0;
		const float theta = static_cast<float>(timer.DeltaTime() * radiansPerSecond);

		if (m_isInConstantRotationLeft)
			RotateAroundLookAtPointX(theta);
		else if (m_isInConstantRotationRight)
			RotateAroundLookAtPointX(-theta);

		if (m_isInConstantRotationUp)
			RotateAroundLookAtPointY(theta);
		else if (m_isInConstantRotationDown)
			RotateAroundLookAtPointY(-theta);

		if (m_isInConstantRotationClockwise)
			RotateAroundLookAtPointClockwise(theta);
		else if (m_isInConstantRotationCounterClockwise)
			RotateAroundLookAtPointClockwise(-theta);
	}

	// Make sure to try to update the view matrix in case anything changed
	UpdateViewMatrix();
}

void Camera::StartAnimatedMove(float duration, const XMFLOAT3& finalPosition, const XMFLOAT3& finalUp, const XMFLOAT3& finalLookAt) noexcept
{
	ASSERT(duration > 0.0, "Animated duration must not be negative");

	// Don't start an animation if we are in constant rotation
	if (IsInConstantRotation())
		return;

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

void Camera::StartConstantLeftRotation() noexcept
{
	// Don't start a left rotation if we are already rotating right
	if (!m_isInConstantRotationRight)
	{
		m_isInConstantRotationLeft = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
}
void Camera::StartConstantRightRotation() noexcept
{
	// Don't start a right rotation if we are already rotating left
	if (!m_isInConstantRotationLeft)
	{
		m_isInConstantRotationRight = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
}
void Camera::StartConstantUpRotation() noexcept
{
	// Don't start an up rotation if we are already rotating down
	if (!m_isInConstantRotationDown)
	{
		m_isInConstantRotationUp = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
}
void Camera::StartConstantDownRotation() noexcept
{
	// Don't start a down rotation if we are already rotating up
	if (!m_isInConstantRotationUp)
	{
		m_isInConstantRotationDown = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
}
void Camera::StartConstantClockwiseRotation() noexcept
{
	// Don't start a clockwise rotation if we are already rotating counter clockwise
	if (!m_isInConstantRotationCounterClockwise)
	{
		m_isInConstantRotationClockwise = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
}
void Camera::StartConstantCounterClockwiseRotation() noexcept
{
	// Don't start a counter clockwise rotation if we are already rotating clockwise
	if (!m_isInConstantRotationClockwise)
	{
		m_isInConstantRotationCounterClockwise = true;
		m_performingAnimatedMove = false; // Cancel current animation
	}
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

void Camera::CenterOnFace() noexcept
{
	if (IsInConstantRotation())
		return;

	XMFLOAT3 newPos;
	XMFLOAT3 newUp;

	XMVECTOR position = XMLoadFloat3(&m_position);
	XMVECTOR up = XMLoadFloat3(&m_up);
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt);

	float length = XMVectorGetX(XMVector3Length(lookAt - position)); 

	// Determine which element has the largest magnitude, set the others equal to 0 and set it to the current distance from position to lookAt point
	float abs_x = std::abs(m_position.x);
	float abs_y = std::abs(m_position.y);
	float abs_z = std::abs(m_position.z);
	newPos.x = (abs_x < abs_y || abs_x < abs_z) ? 0.0f : length;
	newPos.y = (abs_y < abs_x || abs_y < abs_z) ? 0.0f : length; 
	newPos.z = (abs_z < abs_x || abs_z < abs_y) ? 0.0f : length; 

	newPos.x *= (m_position.x < 0.0f) ? -1.0f : 1.0f; 
	newPos.y *= (m_position.y < 0.0f) ? -1.0f : 1.0f; 
	newPos.z *= (m_position.z < 0.0f) ? -1.0f : 1.0f;

	// Determine the coordinate with the max value and 0 out the other ones
	// Whichever coordinate for the eye target is used must not be used for the up target, so zero it out
	float xInit = (newPos.x == 0.0f) ? m_up.x : 0.0f;
	float yInit = (newPos.y == 0.0f) ? m_up.y : 0.0f;
	float zInit = (newPos.z == 0.0f) ? m_up.z : 0.0f;

	length = XMVectorGetX(XMVector3Length(up));

	abs_x = std::abs(xInit);
	abs_y = std::abs(yInit);
	abs_z = std::abs(zInit);
	newUp.x = (abs_x < abs_y || abs_x < abs_z) ? 0.0f : length; 
	newUp.y = (abs_y < abs_x || abs_y < abs_z) ? 0.0f : length; 
	newUp.z = (abs_z < abs_x || abs_z < abs_y) ? 0.0f : length; 
	
	newUp.x *= (xInit < 0.0f) ? -1.0f : 1.0f; 
	newUp.y *= (yInit < 0.0f) ? -1.0f : 1.0f; 
	newUp.z *= (zInit < 0.0f) ? -1.0f : 1.0f; 

	// Translate the position to account for when the lookAt point is not the origin
	newPos.x += m_lookAt.x;
	newPos.y += m_lookAt.y;
	newPos.z += m_lookAt.z;

	StartAnimatedMove(0.4f, newPos, newUp);
}
void Camera::Start90DegreeRotationLeft(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	XMFLOAT3 newPosition = ComputePositionAfterLeftRightRotation(DirectX::XM_PIDIV2);
	StartAnimatedMove(duration, newPosition);
}
void Camera::Start90DegreeRotationRight(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	XMFLOAT3 newPosition = ComputePositionAfterLeftRightRotation(-DirectX::XM_PIDIV2);
	StartAnimatedMove(duration, newPosition);
}
void Camera::Start90DegreeRotationUp(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	auto [position, up] = ComputePositionAndUpAfterUpDownRotation(DirectX::XM_PIDIV2);
	StartAnimatedMove(duration, position, up);
}
void Camera::Start90DegreeRotationDown(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	auto [position, up] = ComputePositionAndUpAfterUpDownRotation(-DirectX::XM_PIDIV2);
	StartAnimatedMove(duration, position, up);
}
void Camera::Start90DegreeRotationClockwise(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	XMFLOAT3 newUp;

	XMVECTOR pos = XMLoadFloat3(&m_position);
	XMVECTOR up = XMLoadFloat3(&m_up);
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt);
	pos = XMVector3Normalize(lookAt - pos);

	up = XMVector3Normalize(RotateVector(up, pos, DirectX::XM_PIDIV2));
	XMStoreFloat3(&newUp, up);

	StartAnimatedMove(duration, m_position, newUp);
}
void Camera::Start90DegreeRotationCounterClockwise(float duration) noexcept
{
	ASSERT(duration > 0.0f, "When doing a 90 degree rotation, the duration should always be > 0");

	if (IsInConstantRotation())
		return;

	XMFLOAT3 newUp;

	XMVECTOR pos = XMLoadFloat3(&m_position);
	XMVECTOR up = XMLoadFloat3(&m_up);
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt);
	pos = XMVector3Normalize(lookAt - pos);

	up = XMVector3Normalize(RotateVector(up, pos, -DirectX::XM_PIDIV2));
	XMStoreFloat3(&newUp, up);

	StartAnimatedMove(duration, m_position, newUp);
}


XMFLOAT3 Camera::ComputePositionAfterLeftRightRotation(float theta) const noexcept
{
	XMFLOAT3 result; 

	XMVECTOR newPos = XMLoadFloat3(&m_position); 
	XMVECTOR up = XMLoadFloat3(&m_up); 
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt); 

	newPos -= lookAt;
	newPos = RotateVector(newPos, up, theta);
	newPos += lookAt;

	XMStoreFloat3(&result, newPos);
	return result;
}
std::tuple<XMFLOAT3, XMFLOAT3> Camera::ComputePositionAndUpAfterUpDownRotation(float theta) const noexcept
{
	XMFLOAT3 newPosition, newUp; 

	XMVECTOR newPos = XMLoadFloat3(&m_position); 
	XMVECTOR up = XMLoadFloat3(&m_up); 
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt); 

	newPos -= lookAt; 
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(newPos, up)); 

	newPos = RotateVector(newPos, right, theta);
	newPos += lookAt;

	XMStoreFloat3(&newPosition, newPos);

	up = XMVector3Normalize(XMVector3Cross(right, newPos)); 
	XMStoreFloat3(&newUp, up);

	return { newPosition, newUp };
}
XMFLOAT3 Camera::ComputeUpAfterClockwiseRotation(float theta) const noexcept
{
	XMFLOAT3 result;

	XMVECTOR pos = XMLoadFloat3(&m_position);
	XMVECTOR up = XMLoadFloat3(&m_up);
	XMVECTOR lookAt = XMLoadFloat3(&m_lookAt);
	pos = XMVector3Normalize(lookAt - pos);

	up = XMVector3Normalize(RotateVector(up, pos, theta));

	XMStoreFloat3(&result, up);
	return result;
}


}