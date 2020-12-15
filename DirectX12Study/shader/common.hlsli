cbuffer passConstant:register (b0)
{
	matrix g_viewproj;		// projecting object to window space matrix
	vector g_lightPos;
	matrix g_shadow;
	matrix g_lightViewProj;
}

///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
	uint instanceID:SV_InstanceID;
	float4 lvpos : POSITION1;
};

