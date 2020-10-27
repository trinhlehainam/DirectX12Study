#include "common.hlsli"

cbuffer Matrix:register (b0)
{
	matrix world;
	matrix viewproj;
	//matrix mat;
}

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD)
{
	VsOutput ret;
	pos = mul(mul(viewproj,world), pos);
	ret.svpos = pos;
	ret.pos = pos;
	ret.uv = uv;
	return ret;
}