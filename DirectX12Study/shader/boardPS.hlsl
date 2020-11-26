#include "boardcommon.hlsli"

Texture2D<float4> postEffectTex:register (t0);
Texture2D<float4> normalMapTex:register (t1);
SamplerState smp : register (s0);

float4 boardPS(BoardOutput input) : SV_TARGET
{
	float w,h,level;
postEffectTex.GetDimensions(0, w, h, level);
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
		(postEffectTex.Sample(smp, input.uv) +
			postEffectTex.Sample(smp, input.uv + float2(dt.x, dt.y))*-2 +						// bottom right
			postEffectTex.Sample(smp, input.uv + float2(-dt.x, -dt.y))*2 +					// top left
			postEffectTex.Sample(smp, input.uv + float2(0, dt.y))*-1 +							// bottom
			postEffectTex.Sample(smp, input.uv + float2(0, -dt.y)) +						// top
			postEffectTex.Sample(smp, input.uv + float2(dt.x, 0))*-1 +							// right
			postEffectTex.Sample(smp, input.uv + float2(-dt.x, 0)));						// left

	float2 normalMap = normalMapTex.Sample(smp, input.uv).xy;
	normalMap = normalMap * 2.f - 1.f;

	return postEffectTex.Sample(smp, input.uv + normalMap*0.01f);
}