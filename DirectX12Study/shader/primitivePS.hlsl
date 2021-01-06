
#define NUM_DIRECTIONAL_LIGHT 3
#define NUM_POINT_LIGHT 0
#define NUM_SPOT_LIGHT 0

#include "common.hlsli"
#include "primitiveCommon.hlsli"

Texture2D<float> g_shadowTex:register(t0);
SamplerState g_smpBorder:register(s0);

cbuffer Materials : register(b2)
{
	float4 g_diffuseAlbedo;
	float3 g_fresnelF0;
	float g_roughness;
}

struct PSOutput
{
	float4 rtTexColor : SV_TARGET0;
	float4 rtNormalTexColor : SV_TARGET1;
};

PSOutput primitivePS(PrimitiveOut input)
{
	PSOutput ret;
	Material material = { g_diffuseAlbedo, g_fresnelF0, 1.0f - g_roughness };
	float4 ambient = g_ambientLight * g_diffuseAlbedo;
	float4 color = ComputeLighting(g_lights, material, input.pos.xyz, g_viewPos, input.normal.xyz);
	// convert NDC to TEXCOORD
	float2 uv = (input.lvpos.xy + float2(1,-1)) * float2(0.5, -0.5);
	
	ret.rtTexColor = color;
	static const float bias = 0.005f;
	if (input.lvpos.z - bias > g_shadowTex.Sample(g_smpBorder, uv))
	{
		//ret.rtTexColor = float4(color.rgb * 0.3, 1);
	}
	
	ret.rtNormalTexColor = float4(input.normal.xyz, 1);

	return ret;
}