#include "common.hlsli"

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD, float4 normal : NORMAL)
{
	VsOutput ret;
	pos = mul(mul(viewproj,world), pos);
	
	ret.pos = pos;
	ret.svpos = pos;
	matrix warudo = world;
	warudo._14_24_34 = 0.0f;		// ���s�ړ���������
	ret.norm = mul(world,normal);
	ret.uv = uv;
	return ret;
}