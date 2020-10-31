#include "common.hlsli"

Texture2D<float4> tex:register(t0);
SamplerState	  smp:register(s0);

// Pixel Shader
// return color to texture
float4 PS(VsOutput input) : SV_TARGET
{
	float3 lightPos = normalize(float3(1,1,1));
	// Return Object`s color
	// Convert -1 ~ 1 -> 0 ~ 1
	// Solution 
	// (-1 ~ 1) + 1 -> (0 ~ 2)
	// (0 ~ 2) / 2 -> (0 ~ 1)
	// calculate light with cos
	float brightness = dot(input.norm.xyz, lightPos);
	float3 eyePos = float3(0.0f, 10.0f, 15.0f);
	float3 eyeRay = normalize(eyePos - input.pos);
	float3 lightRay = reflect(-lightPos, input.norm.xyz);
	float specularity = saturate(pow(saturate(dot(eyeRay, lightRay)), specular));
	
	return float4(brightness * diffuse + specular.rgb* specularity, alpha);
	//return tex.Sample(smp,input.uv);
}