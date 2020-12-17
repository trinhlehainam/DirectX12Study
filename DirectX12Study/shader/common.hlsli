cbuffer passConstant:register (b0)
{
	vector g_viewPos;
	float4x4 g_viewproj;		// projecting object to window space matrix
	float3 g_lightPos;
	float4x4 g_lightViewProj;
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

