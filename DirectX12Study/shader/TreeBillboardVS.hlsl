struct VertexOutput
{
	float3 pos : POSITION;
	float2 size : SIZE;
};

VertexOutput TreeBillboardVS(float3 pos : POSITION, float2 size : SIZE)
{
	VertexOutput ret;
	ret.pos = pos;
	ret.size = size;
	return ret;
}