cbuffer Matrix:register (b0)
{
	matrix world;			// transform to world space matrix
	matrix viewproj;		// projecting object to window space matrix
	vector lightPos;
	matrix shadow;
	matrix lightViewProj;
}

cbuffer Bones:register (b1)
{
	matrix bones[512];
}

///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
	uint instanceID:SV_InstanceID;
};

struct VsInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float4 normal : NORMAL;
	min16uint2 boneno : BONENO;
	float weight : WEIGHT;
	uint instanceID:SV_InstanceID;
};