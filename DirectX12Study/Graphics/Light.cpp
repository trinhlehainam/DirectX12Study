#include "Light.h"

using namespace DirectX;

Light::Light():
	Strength(XMFLOAT3( 1.0f, 1.0f, 1.0f )), 
	FallOffStart(1.0f), 
	Direction(XMFLOAT3( 0.0f, -1.0f, 0.0f )),
    FallOffEnd(100.0f), 
	Position(XMFLOAT3( 0.0f, 0.0f, 0.0f )), 
	SpotPower(64.0f)
{
}

Light::Light(DirectX::XMFLOAT3 Strength_, DirectX::XMFLOAT3 Direction_, DirectX::XMFLOAT3 Position_,
	float FallOfStart_, float FallOfEnd_, float SpotPower_):
	Strength(Strength_), Direction(Direction_), Position(Position_),
	FallOffStart(FallOfStart_), FallOffEnd(FallOfEnd_), SpotPower(SpotPower_)
{
}
