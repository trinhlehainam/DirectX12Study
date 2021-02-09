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
	matrix g_texTransform;
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

	skinMat._14_24_34 = 0.0f;		// remove translation of matrix
	ret.norm = mul(g_world, mul(skinMat, input.normal)); // normal vector DOESN'T TRANSLATE position

#if SHADOW_PIPELINE
	ret.svpos = mul(g_lights[0].ProjectMatrix, ret.pos);
#else
	ret.svpos = mul(g_viewproj, ret.pos);
#endif
	
	ret.lvpos = mul(g_lights[0].ProjectMatrix, ret.pos);
	ret.uv = input.uv;
	ret.instanceID = input.instanceID;
	return ret;
}