#include "LightingUtils.hlsli"

cbuffer worldPass:register (b0)
{
	float4x4 g_viewproj; // projecting object to window space matrix
	float3 g_viewPos;
	float padding0;
	float4 g_ambientLight;
	float4 g_fogColor;
	float g_fogStart;
	float g_fogRange;
	float g_focusStart;
	float g_focusRange;
	
	Light g_lights[MAX_LIGHTS];
}



