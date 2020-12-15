Texture2D<float> g_shadowTex:register(t0);
SamplerState g_smpBorder:register(s0);

struct PrimitiveOut
{
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float4 lvpos : POSITION1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

float4 primitivePS(PrimitiveOut input) : SV_TARGET
{
	
	float4 color = float4(1,1,1,1);
	// convert NDC to TEXCOORD
	float2 uv = (input.lvpos.xy + float2(1,-1)) * float2(0.5, -0.5);
	
	const float bias = 0.005f;
	if (input.lvpos.z - bias > g_shadowTex.Sample(g_smpBorder, uv))
	{
		return float4(color.rgb*0.3, 1);
	}
	return color;
}