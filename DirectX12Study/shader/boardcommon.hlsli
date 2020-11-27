struct BoardOutput
{
	float4 svpos:SV_POSITION;
	float2 uv:TEXCOORD;
};

cbuffer Time:register (b0)
{
	float time;
}