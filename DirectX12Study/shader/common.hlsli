#include "LightingUtils.hlsli"

cbuffer worldPass:register (b0)
{
	float4x4 g_viewproj; // projecting object to window space matrix
	float3 g_viewPos;
	float padding0;
	float4 g_ambientLight;
	
	Light g_lights[MAX_LIGHTS];
}



