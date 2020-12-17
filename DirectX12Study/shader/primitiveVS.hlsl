#include "common.hlsli"
#include "primitiveCommon.hlsli"

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
	//ret.pos = mul(world, input.pos);
	
	// Remove translate
	//matrix wrl = world;
	//wrl._14_24_34 = 0.0f;
	//ret.normal = mul(wrl, input.normal);
	
	// test
	ret.pos = input.pos;
	ret.normal = input.normal;
	//
	
	ret.tangent = input.tangent;
	ret.uv = input.uv;
	ret.svpos = mul(g_viewproj, ret.pos);
	ret.lvpos = mul(g_lightViewProj, ret.pos);
	
	return ret;
}