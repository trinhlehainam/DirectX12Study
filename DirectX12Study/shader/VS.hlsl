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
	matrix g_world; // transform to world space matrix
	matrix g_bones[512];
}

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( VsInput input )
{
	VsOutput ret;
	
	matrix skinMat = g_bones[input.boneno.x] * input.weight + g_bones[input.boneno.y] * (1.0f - input.weight);
	ret.pos = mul(g_world, mul(skinMat, input.pos));
	
	ret.svpos = mul(g_viewproj, ret.pos);

	ret.lvpos = mul(g_lightViewProj, ret.pos);

	skinMat._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(g_world, mul(skinMat, input.normal)); // normal vector DOESN'T TRANSLATE position

	ret.uv = input.uv;
	ret.instanceID = input.instanceID;
	return ret;
}