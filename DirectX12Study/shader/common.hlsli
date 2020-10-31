cbuffer Matrix:register (b0)
{
	matrix world;
	matrix viewproj;
}

// Only Pixel Shader can see it
cbuffer Material:register (b1)
{
	float3 diffuse;
	float alpha;
	float3 specular;
	float specularity;
	float3 ambient;
}

///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
};