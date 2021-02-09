
#define NUM_DIRECTIONAL_LIGHT 3
#define NUM_POINT_LIGHT 0
#define NUM_SPOT_LIGHT 0

#include "common.hlsli"
#include "primitiveCommon.hlsli"

Texture2D<float> g_shadowTex:register(t0);
Texture2D<float4> g_texture : register(t1);
SamplerState g_smpWrap : register(s0);
SamplerState g_smpBorder:register(s1);

cbuffer Materials : register(b2)
{
	float4 g_diffuseAlbedo;
	float3 g_fresnelF0;
	float g_roughness;
}

struct PSOutput
{
	float4 rtTex : SV_TARGET0;
	float4 rtNormalTex : SV_TARGET1;
	float4 rtBrightTex : SV_TARGET2;
};

PSOutput primitivePS(PrimitiveOut input)
{
	PSOutput ret;
	
	float4 diffuseAlbedo = g_texture.Sample(g_smpWrap, input.uv) * g_diffuseAlbedo;
	
#ifdef ALPHA_TEST
	// discard pixel that have zero alpha when sampling texture
	clip(diffuseAlbedo.a - 0.1f);
#endif
	
	Material material = { diffuseAlbedo, g_fresnelF0, 1.0f - g_roughness };
	float4 ambient = g_ambientLight * diffuseAlbedo;
	float4 color = ComputeLighting(g_lights, material, input.pos.xyz, g_viewPos, input.normal.xyz);
	// convert NDC to TEXCOORD
	float2 uv = (input.lvpos.xy + float2(1.0f,-1.0f)) * float2(0.5f, -0.5f);
	
	static const float bias = 0.005f;
	float shadowFactor = 1.0f;
	if (input.lvpos.z - bias > g_shadowTex.Sample(g_smpBorder, uv))
	{
		shadowFactor = 0.3f;
	}
	color.rgb *= shadowFactor;
	
#ifdef FOG
	float clampFogDistance = saturate(distance(g_viewPos, input.pos.xyz) / g_fogRange);
	color = lerp(color, g_fogColor, clampFogDistance);
#endif
	
	ret.rtTex = float4(color.rgb, 1.0f);
	ret.rtNormalTex = float4(input.normal.xyz, 1.0f);
	ret.rtBrightTex = float4(1.0f, 0.0f, 0.0f, 1.0f);

	return ret;
}