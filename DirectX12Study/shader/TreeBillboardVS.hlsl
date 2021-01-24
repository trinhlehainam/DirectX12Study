
struct VertexOutput
{
	float3 pos : POSITION;
	float2 size : SIZE;
};

cbuffer ObjectConstant : register(b1)
{
	float4x4 g_world;
};

VertexOutput TreeBillboardVS(float4 pos : POSITION, float2 size : SIZE)
{
	VertexOutput ret;
	ret.pos = mul(g_world, pos).xyz;
	ret.size = size;
	return ret;
}