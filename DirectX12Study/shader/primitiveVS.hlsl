#include "common.hlsli"
#include "primitiveCommon.hlsli"

cbuffer ObjectConstant : register(b1)
{
	float4x4 g_world;
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
	
	// Test
	//ret.pos = input.pos;
	//ret.normal = input.normal;
	//
	
	ret.tangent = input.tangent;
	ret.uv = input.uv;
	ret.svpos = mul(g_viewproj, ret.pos);
	ret.lvpos = mul(g_lightViewProj, ret.pos);
	
	return ret;
}