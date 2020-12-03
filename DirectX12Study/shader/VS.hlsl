#include "common.hlsli"

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( VsInput input )
{
	VsOutput ret;
	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);

	ret.pos = mul(world, mul(skinMat,input.pos));
	ret.svpos = mul(viewproj, ret.pos);

	ret.lvpos = mul(lightViewProj, ret.pos);

	skinMat._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(world,mul(skinMat,input.normal));		// normal vector DOESN'T TRANSLATE position

	ret.uv = input.uv;
	ret.instanceID = input.instanceID;
	return ret;
}