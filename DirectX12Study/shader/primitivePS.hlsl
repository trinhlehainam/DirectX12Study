#include "primitiveCommon.hlsli"

Texture2D<float> g_shadowTex:register(t0);
SamplerState g_smpBorder:register(s0);

struct VSOutput
{
	float4 color : SV_TARGET0;
	float4 color1 : SV_TARGET1;
};

VSOutput primitivePS(PrimitiveOut input)
{
	VSOutput ret;
	float4 color = float4(1,1,1,1);
	// convert NDC to TEXCOORD
	float2 uv = (input.lvpos.xy + float2(1,-1)) * float2(0.5, -0.5);
	
	const float bias = 0.005f;
	if (input.lvpos.z - bias > g_shadowTex.Sample(g_smpBorder, uv))
	{
		ret.color = float4(color.rgb*0.3, 1);
	}

	ret.color = color;
	ret.color1 = float4(0, 0, 1, 1);

	return ret;
}