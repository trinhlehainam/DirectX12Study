cbuffer passConstant:register (b0)
{
	matrix viewproj;		// projecting object to window space matrix
	vector lightPos;
	matrix shadow;
	matrix lightViewProj;
}

///���_�V�F�[�_�o��
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
	uint instanceID:SV_InstanceID;
	float4 lvpos : POSITION1;
};

