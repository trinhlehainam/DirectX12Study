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
	float3 lightRay = normalize(float3(-1,1,1));
	// calculate light with cos
	// use Lambert function to calculate brightness
	float brightness = dot(input.norm.xyz, lightRay);			

	float4 tn = toon.Sample(toonSmp, brightness);	// toon
	float3 eyePos = float3(10.0f, 10.0f, 30.0f);
	float3 eyeRay = normalize(input.pos.xyz - eyePos);
	float3 refectLight = reflect(lightRay, input.norm.xyz);
	float sat = saturate(pow(saturate(dot(eyeRay, -refectLight)), specularity));	// saturate

	// transform xy coordinate to uv coordinate
	float2 sphereUV = input.norm.xy * float2(0.5, -0.5) + 0.5;

	return float4(max(ambient,tn  * diffuse) + specular * sat , alpha)
		* tex.Sample(smp, input.uv)
		* sph.Sample(smp, sphereUV)
		+ spa.Sample(smp, sphereUV);
}