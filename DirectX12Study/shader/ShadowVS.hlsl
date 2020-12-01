#include "common.hlsli"

VsOutput ShadowVS(float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL, min16uint2 boneno : BONENO, float weight : WEIGHT)
{
	VsOutput ret;
	matrix skinMat = bones[boneno.x] * weight + bones[boneno.y] * (1.0f - weight);
	matrix warudo = mul(world, shadow);
	ret.pos = mul(warudo, mul(skinMat, pos));
	//ret.pos = mul(world,pos);
	ret.svpos = mul(viewproj, ret.pos);
	warudo = world;
	warudo._14_24_34 = 0.0f;		// •½sˆÚ“®¬•ª–³Œø
	skinMat._14_24_34 = 0.0f;		// •½sˆÚ“®¬•ª–³Œø
	ret.norm = mul(warudo, mul(skinMat, normal));
	//ret.norm = mul(warudo,normal);
	ret.uv = uv;
	ret.boneno = boneno;
	ret.weight = weight;
	return ret;
}