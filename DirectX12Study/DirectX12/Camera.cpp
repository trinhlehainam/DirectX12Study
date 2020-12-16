#include "Camera.h"

using namespace DirectX;

void Camera::SetCameraPosition(float x, float y, float z)
{
	m_pos = { x,y,z,1.0f };
}

void Camera::SetTargetPosition(float x, float y, float z)
{
	m_targetPos = { x,y,z,1.0f };
	XMStoreFloat(&m_targetDistance,XMVector4Length(m_targetPos - m_pos));
	m_phi = acosf(m_pos.m128_f32[1]/ m_targetDistance);
}

void Camera::SetFOVAngle(float fovAngle)
{
	m_fovAngle = fovAngle;
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspecRatio = aspectRatio;
}

void Camera::SetViewFrustum(float near, float far)
{
	m_near = near;
	m_far = far;
}

void Camera::Init()
{
	ConvertSphericalToCoordinate();

	m_view = XMMatrixLookAtRH(m_pos, m_targetPos, DEFAULT_UP_VECTOR);
	m_proj = XMMatrixPerspectiveFovRH(m_fovAngle, m_aspecRatio, m_near, m_far);
}

DirectX::XMFLOAT4X4 Camera::GetCameraSpaceMatrix() const
{
	XMFLOAT4X4 cameraSpace;
	XMStoreFloat4x4(&cameraSpace, m_view);
	return cameraSpace;
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionMatrix() const
{
	XMFLOAT4X4 viewProj;
	XMStoreFloat4x4(&viewProj, m_view*m_proj);
	return viewProj;
}

DirectX::XMFLOAT3 Camera::GetCameraPosition() const
{
	XMFLOAT3 cameraPos;
	XMStoreFloat3(&cameraPos, m_pos);
	return cameraPos;
}

void Camera::RotateAroundTarget(float deltaMouseX, float deltaMouseY, float t)
{
	m_theta += XMConvertToRadians(t*deltaMouseX);
	m_phi += XMConvertToRadians(t*deltaMouseY);
}

void Camera::DollyInTarget(float distance)
{
	m_targetDistance -= distance;
}

void Camera::DollyOutTarget(float distance)
{
	m_targetDistance += distance;
}

void Camera::Update()
{
	ConvertSphericalToCoordinate();
	m_view = XMMatrixLookAtRH(m_pos, m_targetPos, DEFAULT_UP_VECTOR);
}

void Camera::ConvertSphericalToCoordinate()
{
	m_pos.m128_f32[0] = m_targetDistance * sinf(m_phi) * cosf(m_theta); // x
	m_pos.m128_f32[1] = m_targetDistance * cosf(m_phi);					// y
	m_pos.m128_f32[2] = m_targetDistance * sinf(m_phi) * sin(m_theta);	// z
}

void Camera::UpdateAfterTransformed()
{
	ConvertSphericalToCoordinate();
}
