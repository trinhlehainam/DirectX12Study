#include "boardcommon.hlsli"

Texture2D<float4> g_renderTargetTex:register (t0);
Texture2D<float4> g_normalMapTex:register (t1);
SamplerState smpWrap:register(s0);
SamplerState smpBorder:register(s1);

cbuffer Time:register (b0)
{
	float g_time;
}

float4 boardPS(BoardOutput input) : SV_TARGET
{

	//float4 color =
	//	(renderTargetTex.Sample(smpWrap, input.uv) +
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(dt.x, dt.y)) +						// bottom right
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(-dt.x, dt.y)) +						// bottom left
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(dt.x, -dt.y)) +						// top right
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(-dt.x, -dt.y)) +					// top left
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(0, dt.y)) +							// bottom
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(0, -dt.y)) +						// top
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(dt.x, 0)) +							// right
	//		renderTargetTex.Sample(smpWrap, input.uv + float2(-dt.x, 0)));						// left
	

	float2 nmUV = input.uv -0.5;
	nmUV /= g_time;
	nmUV += 0.5;

	float4 nmCol = g_normalMapTex.Sample(smpBorder, nmUV);

	float w, h, level;
	g_renderTargetTex.GetDimensions(0, w, h, level);
	float2 dt = float2(1.f / w, 1.f / h);
	float2 offset = ((nmCol.xy * 2) - 1) * nmCol.a;

	//float4 color = renderTargetTex.Sample(smpWrap, input.uv + offset*0.03f);
	float4 color = g_renderTargetTex.Sample(smpWrap, input.uv);
	//return color;
	if (color.a > 0.0f)
	{
		return color;
	}	
	else
	{
		float div = 100.0f;
		return float4(0, 0, 0, 1);
		return float4(fmod(input.uv, 1.0f / div) * div,1, 1);
	}	
}