#include "common.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState	  smp:register(s0);

// Pixel Shader
// return color to texture
float4 PS(VsOutput input) : SV_TARGET
{
	float3 lpos = normalize(float3(1,1,1));
	// Return Object`s color
	// Convert -1 ~ 1 -> 0 ~ 1
	// Solution 
	// (-1 ~ 1) + 1 -> (0 ~ 2)
	// (0 ~ 2) / 2 -> (0 ~ 1)
	float b = dot(input.norm.xyz, lpos);
	return float4(b*diffuse,1);
	//return tex.Sample(smp,input.uv);
}