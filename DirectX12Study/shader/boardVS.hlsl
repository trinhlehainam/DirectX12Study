#include "boardcommon.hlsli"

BoardOutput boardVS(float4 pos : POSITION)
{
	BoardOutput ret;
	ret.svpos = pos;
	ret.uv = (pos.xy * float2(1, -1) + float2(1, -1)) * 0.5f;
	return ret;
}