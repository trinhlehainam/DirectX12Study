#include "common.hlsli"

Texture2D<float4> tex:register(t0);			// main texture
Texture2D<float4> sph:register(t1);			// sphere mapping texture
Texture2D<float4> spa:register(t2);
Texture2D<float4> toon:register(t3);
SamplerState	  smp:register(s0);
SamplerState	  toonSmp:register(s1);

// Pixel Shader
// return color to texture
float4 PS(VsOutput input) : SV_TARGET
{
	float3 lightPos = normalize(float3(-1,1,1));
	// Return Object`s color
	// Convert -1 ~ 1 -> 0 ~ 1
	// Solution 
	// (-1 ~ 1) + 1 -> (0 ~ 2)
	// (0 ~ 2) / 2 -> (0 ~ 1)
	// calculate light with cos
	float brightness = dot(input.norm.xyz, lightPos);
	float4 tn = toon.Sample(toonSmp, brightness);	// toon
	float3 eyePos = float3(0.0f, 10.0f, 30.0f);
	float3 eyeRay = normalize(eyePos - input.pos.xyz);
	float3 lightRay = reflect(-lightPos, input.norm.xyz);
	// saturate
	float sat = saturate(pow(saturate(dot(eyeRay, lightRay)), specularity));
	// transform xy coordinate to uv coordinate
	float2 sphereUV = input.norm.xy * float2(0.5, -0.5) + 0.5;
	
	return float4(max(ambient,tn  * diffuse) + specular * sat , alpha)
		* tex.Sample(smp, input.uv)
		* sph.Sample(smp, sphereUV)
		+ spa.Sample(smp, sphereUV);
	//return tex.Sample(smp,input.uv);
}