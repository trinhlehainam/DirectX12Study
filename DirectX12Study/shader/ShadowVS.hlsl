#include "common.hlsli"

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
	matrix world; // transform to world space matrix
	matrix bones[512];
}

VsOutput ShadowVS(VsInput input)
{
	VsOutput ret;

	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);
	ret.pos = mul(world, mul(skinMat, input.pos));
	ret.svpos = mul(lightViewProj, ret.pos);

	return ret;
}