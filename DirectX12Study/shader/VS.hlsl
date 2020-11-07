#include "common.hlsli"

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL)
{
	VsOutput ret;
	pos = mul(mul(viewproj,world), pos);
	
	ret.pos = pos;
	ret.svpos = pos;
	matrix warudo = world;
	warudo._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(world,normal);
	ret.uv = uv;
	return ret;
}