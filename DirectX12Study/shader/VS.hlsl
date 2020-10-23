#include "common.hlsli"

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD)
{
	VsOutput ret;
	pos.xy /= float2(640, -360);
	pos.xy += float2(-1, 1);
	ret.svpos = pos;
	ret.pos = pos;
	ret.uv = uv;
	return ret;
}