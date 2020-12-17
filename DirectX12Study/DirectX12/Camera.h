#pragma once
#include <DirectXMath.h>

class Camera
{
public:
	Camera() = default;
	~Camera() = default;

	void SetCameraPosition(float x, float y, float z);
	// Set position that camera looking at
	void SetTargetPosition(float x, float y, float z);
	/// <summary>
	/// the range of field that camera can see that defined by angle
	/// </summary>
	/// <param name="fovAngle:">field of view angle (radiant)</param>
	void SetFOVAngle(float fovAngle);
	/// <summary>
	/// Screen's ratio or view's ratio
	/// </summary>
	/// <param name="aspectRatio:">width / height</param>
	void SetAspectRatio(float aspectRatio);
	
	/// <summary>
	/// Set range distance that camera can see
	/// </summary>
	/// <param name="near: ">the distance form near plane to camera</param>
	/// <param name="far: ">the distance form near plane to camera</param>
	void SetViewFrustum(float nearPlane, float farPlane);
	void Init();
public:
	DirectX::XMFLOAT4X4 GetCameraSpaceMatrix() const;
	DirectX::XMFLOAT4X4 GetViewProjectionMatrix() const;
	DirectX::XMFLOAT3 GetCameraPosition() const;
	DirectX::XMFLOAT3 GetTargetPosition() const;

	/// <summary>
	/// rotate camera around target position
	/// </summary>
	/// <param name="deltaMouseX">distance that mouse travled each frame in X-axis</param>
	/// <param name="deltaMouseY">distance that mouse travled each frame in Y-axis</param>
	/// <param name="t">take propotion of distance</param>
	void RotateAroundTarget(float deltaMouseX, float deltaMouseY, float t = 0.25f);

	/// <summary>
	/// Move close to target position
	/// </summary>
	/// <param name="distance :"> moving distance</param>
	void DollyInTarget(float distance);
	/// <summary>
	/// Move far from target position
	/// </summary>
	/// <param name="distance :">moving distance</param>
	void DollyOutTarget(float distance);

	void Update();
private:
	//
	// Vairable use for moving camera
	//
	void ConvertSphericalToCoordinate();
	void UpdateAfterTransformed();

	float m_theta = 0.0f;
	float m_phi = DirectX::XM_PIDIV2;
	// The distance from camera to target position (the position camera looking at)
	float m_targetDistance = 30.0f;

private:
	//
	// Variables use for viewing models
	//

	static constexpr DirectX::XMVECTOR DEFAULT_TARGET_POSITION = { 0.0f, 0.0f, 0.0f, 1.0f };
	static constexpr DirectX::XMVECTOR DEFAULT_UP_VECTOR = { 0.0f, 1.0f, 0.0f, 0.0f };

	DirectX::XMVECTOR m_pos = DirectX::XMVectorZero();
	DirectX::XMVECTOR m_targetPos = DEFAULT_TARGET_POSITION;
	// Matrix covert to camera space (coordinate)
	DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
	// Matrix use for projecting models to camera view
	DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();
	float m_fovAngle = DirectX::XM_PIDIV2;
	float m_aspecRatio = 1.0f;
	float m_near = 1.0f;
	float m_far = 500.0f;
};

