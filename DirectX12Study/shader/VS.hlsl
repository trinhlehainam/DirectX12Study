#include "common.hlsli"

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( VsInput input )
{
	VsOutput ret;
	matrix skinMat = bones[input.boneno.x] * input.weight + bones[input.boneno.y] * (1.0f - input.weight);

	ret.pos = mul(world, mul(skinMat,input.pos));
	ret.svpos = mul(viewproj, ret.pos);

	ret.lvpos = mul(lightViewProj, ret.pos);

	skinMat._14_24_34 = 0.0f;		// ���s�ړ���������
	ret.norm = mul(world,mul(skinMat,input.normal));		// normal vector DOESN'T TRANSLATE position

	ret.uv = input.uv;
	ret.instanceID = input.instanceID;
	return ret;
}