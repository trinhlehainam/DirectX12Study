#include "common.hlsli"
#include "modelcommon.hlsli"

struct VsInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float4 normal : NORMAL;
	min16uint2 boneno : BONENO;
	float weight : WEIGHT;
	uint instanceID : SV_InstanceID;
};

cbuffer objectConstant : register(b1)
{
	matrix g_world; // transform to world space matrix
	matrix g_bones[512];
}

VsOutput ShadowVS(VsInput input)
{
	VsOutput ret;

	matrix skinMat = g_bones[input.boneno.x] * input.weight + g_bones[input.boneno.y] * (1.0f - input.weight);
	ret.pos = mul(g_world, mul(skinMat, input.pos));
	ret.svpos = mul(g_lightViewProj, ret.pos);

	return ret;
}