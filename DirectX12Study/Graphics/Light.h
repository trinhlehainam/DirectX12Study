#pragma once
#include <DirectXMath.h>

struct Light
{
	Light();
	explicit Light(DirectX::XMFLOAT3 Strength, DirectX::XMFLOAT3 Direction, DirectX::XMFLOAT3 Position,
		float FallOfStart, float FallOfEnd, float SpotPower);

	DirectX::XMFLOAT3 Strength;			// Brightness
	float FallOffStart;					// Point light/ spot light
	DirectX::XMFLOAT3 Direction;		// Directional light/ spot light
	float FallOffEnd;					// Point light/ spot light
	DirectX::XMFLOAT3 Position;
	float SpotPower;					// Spot light
};

