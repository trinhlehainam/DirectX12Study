#include "common.hlsli"
#include "primitiveCommon.hlsli"

cbuffer ObjectConstant : register(b1)
{
	float4x4 g_world;
	float4x4 g_texTransform;
}

struct PrimitiveInput
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

PrimitiveOut primitiveVS(PrimitiveInput input)
{
	PrimitiveOut ret;
	
	ret.pos = mul(g_world, input.pos);
	// Remove translate
	matrix wrl = g_world;
	wrl._14_24_34 = 0.0f;
	ret.normal = mul(wrl, input.normal);
	
	ret.tangent = input.tangent;
	ret.uv = mul(g_texTransform, input.uv);
	ret.svpos = mul(g_viewproj, ret.pos);
	ret.lvpos = mul(g_lights[0].ProjectMatrix, ret.pos);
	
	return ret;
}