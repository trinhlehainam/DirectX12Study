#include "Light.h"

Light::Light(DirectX::XMFLOAT3 Strength_, DirectX::XMFLOAT3 Direction_, DirectX::XMFLOAT3 Position_,
	float FallOfStart_, float FallOfEnd_, float SpotPower_):
	Strength(Strength_), Direction(Direction_), Position(Position_),
	FallOfStart(FallOfStart_), FallOfEnd(FallOfEnd_), SpotPower(SpotPower_)
{
}
