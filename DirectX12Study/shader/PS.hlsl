#include "common.hlsli"
#include "modelcommon.hlsli"

Texture2D<float> g_shadowDepthTex : register(t0);
Texture2D<float> g_viewDepthTex : register(t1);
Texture2D<float4> g_tex:register(t2);			// main texture
Texture2D<float4> g_sph:register(t3);			// sphere mapping texture
Texture2D<float4> g_spa:register(t4);
Texture2D<float4> g_toon:register(t5);

SamplerState g_smp:register(s0);
SamplerState g_toonSmp:register(s1);
SamplerComparisonState g_shadowCmpSmp:register(s2);

// Only Pixel Shader can see it
cbuffer Material:register (b2)
{
	float3 g_diffuse;
	float g_alpha;
	float3 g_specular;
	float g_specularity;
	float3 g_ambient;
}

struct PSOutput
{
	float4 rtTexColor : SV_TARGET0;
	float4 rtNormalTexColor : SV_TARGET1;
};

// Pixel Shader
// return color to texture
PSOutput PS(VsOutput input)
{	
	PSOutput ret;
	float3 lightRay = normalize(g_lightPos);
	// calculate light with cos
	/* use Phong method to calculate per-pixel lighting */
	float brightness = dot(input.norm.xyz, lightRay);			

	float4 toon = g_toon.Sample(g_toonSmp, brightness);	// toon
	float3 eyePos = g_viewPos;
	float3 eyeRay = normalize(input.pos.xyz - eyePos);
	float3 refectLight = reflect(lightRay, input.norm.xyz);
	float sat = saturate(pow(saturate(dot(eyeRay, -refectLight)), g_specularity));	// saturate

	// transform xy coordinate to uv coordinate
	float2 sphereUV = input.norm.xy * float2(0.5, -0.5) + 0.5;
	float4 color = float4(max(g_ambient, toon * g_diffuse) + g_specular * sat, g_alpha);
	
	// test
	//float3 testDepth = g_viewDepthTex.Sample(g_smp, input.uv);
	//return float4(testDepth,1);
	//

	const float bias = 0.005f;
	float shadowValue = 1.f;
	float2 shadowUV = (input.lvpos.xy + float2(1, -1)) * float2(0.5, -0.5);

	// PCF (percentage closest filtering)
	shadowValue = g_shadowDepthTex.SampleCmpLevelZero(g_shadowCmpSmp, shadowUV, input.lvpos.z - bias);
	shadowValue = lerp(0.5f, 1.0f, shadowValue);

	// Shadow Depth Offset
	//if (input.lvpos.z - bias > shadowTex.SampleCmp(shadowCmpSmp, uv))
	//{
	//	shadowValue = 0.5f;
	//}

	color *= shadowValue;
	
	ret.rtTexColor = color
		* g_tex.Sample(g_smp, input.uv)
		* g_sph.Sample(g_smp, sphereUV)
		+ g_spa.Sample(g_smp, sphereUV);
	
	ret.rtNormalTexColor = float4(input.norm.xyz, 1);

	return ret;
}