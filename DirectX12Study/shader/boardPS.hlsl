#include "boardcommon.hlsli"

Texture2D<float4> g_rtTex:register (t0);
Texture2D<float4> g_rtNormalTex:register(t1);
Texture2D<float4> g_normalMapTex:register (t2);
Texture2D<float> g_shadowDepthTex:register (t3);
Texture2D<float> g_viewDepthTex:register(t4);

SamplerState smpWrap:register(s0);
SamplerState smpBorder:register(s1);

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
	
	//
	// Normal map
	//
	//float2 nmUV = input.uv -0.5;
	//nmUV /= g_time;
	//nmUV += 0.5;
	//float4 nmCol = g_normalMapTex.Sample(smpBorder, nmUV);
	//float w, h, level;
	//g_renderTargetTex.GetDimensions(0, w, h, level);
	//float2 dt = float2(1.f / w, 1.f / h);
	//float2 offset = ((nmCol.xy * 2) - 1) * nmCol.a;
	//float4 color = renderTargetTex.Sample(smpWrap, input.uv + offset*0.03f);

	// Debug for shadow depth
	if (input.uv.x < 0.25f && input.uv.y < 0.25f)
	{
		float3 shadowColor = g_shadowDepthTex.Sample(smpWrap, input.uv * 4);
		return float4(1.0f - shadowColor, 1);
	}

	// Debug for view depth
	if (input.uv.x < 0.25f && input.uv.y >= 0.25f && input.uv.y < 0.5f)
	{
		float3 viewDepthColor = g_viewDepthTex.Sample(smpWrap, input.uv * 4);
		return float4(viewDepthColor,1);
	}

	// Debug for normal render target
	if (input.uv.x < 0.25f && input.uv.y >= 0.5f && input.uv.y < 0.75f)
	{
		float3 rtNormalColor = g_rtNormalTex.Sample(smpWrap, input.uv * 4);
		return float4(rtNormalColor, 1);
	}
	
	float4 color = g_rtTex.Sample(smpBorder, input.uv);
	//return color;
	if (color.a > 0.0f)
	{
		return color;
	}	
	else
	{
		float div = 100.0f;
		return float4(0.7, 0.9, 0.9, 1);
		return float4(fmod(input.uv, 1.0f / div) * div,1, 1);
	}	
}