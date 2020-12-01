#include "common.hlsli"

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL, min16uint2 boneno : BONENO, float weight : WEIGHT,
	uint instanceID : SV_InstanceID)
{
	VsOutput ret;
	matrix skinMat = bones[boneno.x] * weight + bones[boneno.y] * (1.0f - weight);
	matrix warudo = instanceID == 0 ? world : mul(shadow, world);
	ret.pos = mul(warudo, mul(skinMat,pos));
	//ret.pos = mul(world,pos);
	ret.svpos = mul(viewproj, ret.pos);
	warudo = world;
	warudo._14_24_34 = 0.0f;		// ���s�ړ���������
	skinMat._14_24_34 = 0.0f;		// ���s�ړ���������
	ret.norm = mul(warudo,mul(skinMat,normal));
	//ret.norm = mul(warudo,normal);
	ret.uv = uv;
	ret.boneno = boneno;
	ret.weight = weight;
	ret.instanceID = instanceID;
	return ret;
}