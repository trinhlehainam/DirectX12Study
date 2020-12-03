#include "common.hlsli"

VsOutput ShadowVS(VsInput input)
{
	VsOutput ret;

	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);
	ret.pos = mul(world, mul(skinMat, input.pos));
	ret.svpos = mul(lightViewProj, ret.pos);

	return ret;
}