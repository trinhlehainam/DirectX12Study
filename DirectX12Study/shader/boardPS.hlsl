#include "boardcommon.hlsli"

Texture2D<float4> effectTex:register (t0);
Texture2D<float4> normalMapTex:register (t1);
SamplerState smp : register (s0);

float4 boardPS(BoardOutput input) : SV_TARGET
{
	float w,h,level;
	effectTex.GetDimensions(0, w, h, level);
	float2 dt = float2(1.f / w, 1.f / h);
	//float4 color =
	//	(effectTex.Sample(smp, input.uv) +
	//		effectTex.Sample(smp, input.uv + float2(dt.x, dt.y)) +						// bottom right
	//		effectTex.Sample(smp, input.uv + float2(-dt.x, dt.y)) +						// bottom left
	//		effectTex.Sample(smp, input.uv + float2(dt.x, -dt.y)) +						// top right
	//		effectTex.Sample(smp, input.uv + float2(-dt.x, -dt.y)) +					// top left
	//		effectTex.Sample(smp, input.uv + float2(0, dt.y)) +							// bottom
	//		effectTex.Sample(smp, input.uv + float2(0, -dt.y)) +						// top
	//		effectTex.Sample(smp, input.uv + float2(dt.x, 0)) +							// right
	//		effectTex.Sample(smp, input.uv + float2(-dt.x, 0)));						// left

	float4 color =
		(effectTex.Sample(smp, input.uv) +
			effectTex.Sample(smp, input.uv + float2(dt.x, dt.y))*-2 +						// bottom right
			effectTex.Sample(smp, input.uv + float2(-dt.x, -dt.y))*2 +					// top left
			effectTex.Sample(smp, input.uv + float2(0, dt.y))*-1 +							// bottom
			effectTex.Sample(smp, input.uv + float2(0, -dt.y)) +						// top
			effectTex.Sample(smp, input.uv + float2(dt.x, 0))*-1 +							// right
			effectTex.Sample(smp, input.uv + float2(-dt.x, 0)));						// left

	return normalMapTex.Sample(smp, input.uv);
	return float4(color.rgb,color.a);
}