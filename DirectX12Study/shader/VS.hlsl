///���_�V�F�[�_�o��
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
};

/// Vertex shader
///��{���_�V�F�[�_
/// @param pos ���_���W
/// @return �V�X�e�����_���W
VsOutput VS( float4 pos : POSITION )
{
	VsOutput ret;
	ret.svpos = pos;
	ret.pos = pos;
	return ret;
}