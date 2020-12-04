#include "common.hlsli"

Texture2D<float4> tex:register(t0);			// main texture
Texture2D<float4> sph:register(t1);			// sphere mapping texture
Texture2D<float4> spa:register(t2);
Texture2D<float4> toon:register(t3);
Texture2D<float> shadowTex:register(t4);
SamplerState	  smp:register(s0);
SamplerState	  toonSmp:register(s1);
SamplerComparisonState	  shadowCmpSmp:register(s2);

// Only Pixel Shader can see it
cbuffer Material:register (b2)
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularity;
	float3 ambient;
}

// Pixel Shader
// return color to texture
float4 PS(VsOutput input) : SV_TARGET
{
	float3 lightRay = normalize(lightPos);
	// calculate light with cos
	// use Lambert function to calculate brightness
	float brightness = dot(input.norm.xyz, lightRay);			

	float4 tn = toon.Sample(toonSmp, brightness);	// toon
	float3 eyePos = float3(10.0f, 10.0f, 10.0f);
	float3 eyeRay = normalize(input.pos.xyz - eyePos);
	float3 refectLight = reflect(lightRay, input.norm.xyz);
	float sat = saturate(pow(saturate(dot(eyeRay, -refectLight)), specularity));	// saturate

	// transform xy coordinate to uv coordinate
	float2 sphereUV = input.norm.xy * float2(0.5, -0.5) + 0.5;

	const float bias = 0.005f;
	float shadowValue = 1.f;
	float2 uv = (input.lvpos.xy + float2(1, -1)) * float2(0.5, -0.5);

	// PCF (percentage closest filtering)
	shadowValue = shadowTex.SampleCmpLevelZero(shadowCmpSmp,uv, input.lvpos.z - bias);
	shadowValue = lerp(0.5f, 1.0f, shadowValue);

	// Shadow Depth Offset
	//if (input.lvpos.z - bias > shadowTex.SampleCmp(shadowCmpSmp, uv))
	//{
	//	shadowValue = 0.5f;
	//}

	float4 bright = float4(max(ambient, tn * diffuse) + specular * sat, alpha);
	bright *= shadowValue;
	
	return bright
	* tex.Sample(smp, input.uv)
	* sph.Sample(smp, sphereUV)
	+ spa.Sample(smp, sphereUV);
}