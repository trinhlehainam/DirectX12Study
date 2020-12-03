Texture2D<float> shadowTex:register(t0);
SamplerState smpBorder:register(s0);

struct PrimitiveOut
{
	float4 svpos: SV_POSITION;
	float4 pos: POSITION;
	float4 lvpos : POSITION1;
};

float4 primitivePS(PrimitiveOut input) : SV_TARGET
{
	float2 uv = (input.lvpos.xy + float2(1,-1)) * float2(0.5, -0.5);
	if (input.lvpos.z > shadowTex.Sample(smpBorder, uv))
	{
		return float4(0.3, 0.3, 0.3, 1);
	}
	return float4(1,1,1,1);
}