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
	float4 color = float4(1,1,1,1);
	float2 uv = (input.lvpos.xy + float2(1,-1)) * float2(0.5, -0.5);
	const float bias = 0.005f;
	if (input.lvpos.z - bias > shadowTex.Sample(smpBorder, uv))
	{
		return float4(color.rgb*0.3, 1);
	}
	return color;
}