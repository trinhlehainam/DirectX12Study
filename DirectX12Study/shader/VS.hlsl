#include "common.hlsli"

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL, min16uint2 boneno : BONENO, float weight : WEIGHT)
{
	VsOutput ret;
	matrix transMat = bones[boneno.x] * weight + bones[boneno.y] * (1.0f - weight);
	matrix warudo = mul(world, shadow);
	ret.pos = mul(warudo, mul(transMat,pos));
	ret.svpos = mul(viewproj, ret.pos);
	warudo = world;
	warudo._14_24_34 = 0.0f;		// 平行移動成分無効
	transMat._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(warudo,mul(transMat,normal));
	ret.uv = uv;
	ret.boneno = boneno;
	ret.weight = weight;
	return ret;
}