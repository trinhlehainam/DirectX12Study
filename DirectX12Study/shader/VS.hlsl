#include "common.hlsli"

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( VsInput input )
{
	VsOutput ret;
	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);
	matrix warudo = input.instanceID == 0 ? world : mul(shadow, world);
	ret.pos = mul(warudo, mul(skinMat,input.pos));
	ret.svpos = mul(lightViewProj, ret.pos);

	warudo._14_24_34 = 0.0f;		// 平行移動成分無効
	skinMat._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(warudo,mul(skinMat,input.normal));		// normal vector DOESN'T TRANSLATE position

	ret.uv = input.uv;
	ret.instanceID = input.instanceID;
	return ret;
}