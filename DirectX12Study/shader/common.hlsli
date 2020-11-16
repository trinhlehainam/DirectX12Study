cbuffer Matrix:register (b0)
{
	matrix world;
	matrix viewproj;
}

cbuffer Bones:register (b1)
{
	matrix bones[512];
}

// Only Pixel Shader can see it
cbuffer Material:register (b2)
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularity;
	float3 ambient;
}

///���_�V�F�[�_�o��
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
	min16uint2 boneno:BONENOl;
	float weight:WEIGHT;
};