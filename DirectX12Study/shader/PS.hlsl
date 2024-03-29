#define NUM_DIRECTIONAL_LIGHT 3
#define NUM_POINT_LIGHT 0
#define NUM_SPOT_LIGHT 0

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
	float4 rtTex : SV_TARGET0;
	float4 rtNormalTex : SV_TARGET1;
	float4 rtBrightTex : SV_TARGET2;
	float4 rtFocusTex : SV_TARGET3;
};

// Pixel Shader
// return color to texture
PSOutput PS(VsOutput input)
{
	PSOutput ret;
	float3 lightRay = normalize(g_lights[0].Direction);
	// calculate light with cos
	/* use Phong method to calculate per-pixel lighting */
	float brightness = dot(input.norm.xyz, lightRay);

	float4 toon = g_toon.Sample(g_toonSmp, brightness); // toon
	
	Material material = { toon * float4(g_diffuse, 1.0f), g_specular, g_specularity / 256.0f };
	float4 color = float4(g_ambient * g_diffuse, g_alpha) + ComputeLighting(g_lights, material, input.pos.xyz, g_viewPos, input.norm.xyz);
	
	//
	/*-------------SHADOW---------------*/
	//
	const float bias = 0.005f;
	float shadowValue = 1.f;
	float2 shadowUV = (input.lvpos.xy + float2(1, -1)) * float2(0.5, -0.5);

	// PCF (percentage closest filtering)
	shadowValue = g_shadowDepthTex.SampleCmpLevelZero(g_shadowCmpSmp, shadowUV, input.lvpos.z - bias);
	shadowValue = lerp(0.5f, 1.0f, shadowValue);
	//
	//

	color *= shadowValue;
	
	// transform xy coordinate to uv coordinate
	float2 sphereUV = input.norm.xy * float2(0.5f, -0.5f) + 0.5f;
	
#ifdef FOG
	float clampFogDistance = saturate((distance(g_viewPos, input.pos.xyz) - g_fogStart) / g_fogRange);
	color = lerp(color, g_fogColor, clampFogDistance);
#endif
	
	ret.rtTex = color
		* g_tex.Sample(g_smp, input.uv)
		* g_sph.Sample(g_smp, sphereUV)
		+ g_spa.Sample(g_smp, sphereUV);
	
	ret.rtNormalTex = float4(input.norm.xyz, 1.0f);
	
	float bright_check = dot(float3(0.299f, 0.587f, 0.114f), ret.rtTex.rgb);
	ret.rtBrightTex = bright_check > 0.99f ? ret.rtTex : 0.0f;

	float look_at_distance = mul(g_viewproj, input.pos - float4(g_viewPos, 1.0f)).z;
	look_at_distance = (look_at_distance - g_focusStart) / g_focusRange;
	ret.rtFocusTex = look_at_distance < 0.0f || look_at_distance > 1.0f ? 0.0f : ret.rtTex;
	
	return ret;
}