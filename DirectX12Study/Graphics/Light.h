#pragma once
#include <DirectXMath.h>

struct Light
{
	Light() = default;
	explicit Light(DirectX::XMFLOAT3 Strength, DirectX::XMFLOAT3 Direction, DirectX::XMFLOAT3 Position,
		float FallOfStart, float FallOfEnd, float SpotPower);

	DirectX::XMFLOAT3 Strength;			// Brightness
	float FallOfStart;					// Point light/ spot light
	DirectX::XMFLOAT3 Direction;		// Directional light/ spot light
	float FallOfEnd;					// Point light/ spot light
	DirectX::XMFLOAT3 Position;
	float SpotPower;					// Spot light
};

