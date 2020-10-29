cbuffer Matrix:register (b0)
{
	matrix world;
	matrix viewproj;
	float3 diffuse;
}

///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
	float4 norm:NORMAL;
	float2 uv:TEXCOORD;
};