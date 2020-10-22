#include "common.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState	  smp:register(s0);

// Pixel Shader
// return color wanted to draw
float4 PS(VsOutput input) : SV_TARGET
{
	// Return Object`s color
	// Convert -1 ~ 1 -> 0 ~ 1
	// Solution 
	// (-1 ~ 1) + 1 -> (0 ~ 2)
	// (0 ~ 2) / 2 -> (0 ~ 1)
	return tex.Sample(smp,input.uv);
}