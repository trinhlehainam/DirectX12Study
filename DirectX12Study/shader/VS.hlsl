#include "common.hlsli"

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL, min16uint2 boneno : BONENO, float weight : WEIGHT)
{
	VsOutput ret;
	matrix transMat = bones[boneno.x] * weight + bones[boneno.y] * (1.0f - weight);
	ret.pos = mul(world, mul(transMat,pos));
	ret.svpos = mul(viewproj, ret.pos);
	matrix warudo = world;
	warudo._14_24_34 = 0.0f;		// ���s�ړ���������
	transMat._14_24_34 = 0.0f;		// ���s�ړ���������
	ret.norm = mul(warudo,mul(transMat,normal));
	ret.uv = uv;
	ret.boneno = boneno;
	ret.weight = weight;
	return ret;
}