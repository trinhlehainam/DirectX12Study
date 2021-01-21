
#include "common.hlsli"

Texture2D<float4> g_tex : register(t0);
SamplerState g_smpWrap : register(s0);

cbuffer ObjectConstant : register(b0)
{
	float4x4 g_world;
};

struct GSOutput
{
	float4 svpos : SV_POSITION;
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PixelOutput
{
	float4 color : SV_TARGET0;
	float4 normalColor : SV_TARGET1;
};

PixelOutput TreeBillboardPS(GSOutput input)
{
	PixelOutput ret;
	float4 color = g_tex.Sample(g_smpWrap, input.uv);
	clip(color.a - 0.1f);
	ret.color = color;
	ret.normalColor = float4(input.normal, 1.0f);
	return ret;
}