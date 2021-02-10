#include "common.hlsli"

Texture2D<float4> g_tex : register(t0);
SamplerState g_smpWrap : register(s0);


struct GSOutput
{
	float4 svpos : SV_POSITION;
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PixelOutput
{
	float4 rtTex : SV_TARGET0;
	float4 rtNormalTex : SV_TARGET1;
	float4 rtBrightTex : SV_TARGET2;
	float4 rtFocusTex : SV_TARGET3;
};

PixelOutput TreeBillboardPS(GSOutput input)
{
	PixelOutput ret;
	float4 color = g_tex.Sample(g_smpWrap, input.uv);
	clip(color.a - 0.1f);
#ifdef FOG
	float clampFogDistance = saturate((distance(g_viewPos, input.pos.xyz) - g_fogStart) / g_fogRange);
	color = lerp(color, g_fogColor, clampFogDistance);
#endif
	ret.rtTex = color;
	ret.rtNormalTex = float4(input.normal, 1.0f);
	float bright_check = dot(float3(0.299f, 0.587f, 0.114f), ret.rtTex.rgb);
	ret.rtBrightTex = bright_check > 0.99f ? ret.rtTex : 0.0f;
	
	float look_at_distance = mul(g_viewproj, input.pos - float4(g_viewPos, 1.0f)).z;
	look_at_distance = (look_at_distance - g_focusStart) / g_focusRange;
	ret.rtFocusTex = look_at_distance < 0.0f || look_at_distance > 1.0f ? 0.0f : ret.rtTex;
	return ret;
}