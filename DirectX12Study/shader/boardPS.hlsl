#include "boardcommon.hlsli"

float4 boardPS(BoardOutput input) : SV_TARGET
{
	return float4(input.uv, 1.0f, 1.0f);
}