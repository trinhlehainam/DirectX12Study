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
};

PixelOutput TreeBillboardPS(GSOutput input)
{
	PixelOutput ret;
	float4 color = g_tex.Sample(g_smpWrap, input.uv);
	clip(color.a - 0.1f);
	ret.rtTex = color;
	ret.rtNormalTex = float4(input.normal, 1.0f);
	ret.rtBrightTex = float4(1.0f, 0.0f, 0.0f, 1.0f);
	return ret;
}