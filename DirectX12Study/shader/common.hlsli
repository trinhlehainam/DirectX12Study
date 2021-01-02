struct Light
{
	float3 Strength;
	float FallOfStart;
	float3 Direction;
	float FallOfEnd;
	float3 Position;
	float SpotPower;
};

static const uint MAX_LIGHT = 16;

cbuffer worldPass:register (b0)
{
	float4x4 g_viewproj; // projecting object to window space matrix
	float4x4 g_lightViewProj;
	float3 g_viewPos;
	float padding0;
	
	Light g_light[MAX_LIGHT];
}



