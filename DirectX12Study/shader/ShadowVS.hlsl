#include "common.hlsli"

VsOutput ShadowVS(VsInput input)
{
	VsOutput ret;
	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);
	matrix warudo = mul(world, shadow);
	ret.pos = mul(warudo, mul(skinMat, input.pos));
	ret.svpos = mul(viewproj, ret.pos);

	warudo._14_24_34 = 0.0f;		// 平行移動成分無効
	skinMat._14_24_34 = 0.0f;		// 平行移動成分無効
	ret.norm = mul(warudo, mul(skinMat, input.normal));

	ret.uv = input.uv;
	return ret;
}