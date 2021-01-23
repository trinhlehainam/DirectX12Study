
cbuffer RootConstant : register(b0)
{
	int g_blurRadius;
	float g_weights[11];
}

#define N 256
static const int max_blur_radius = 5;
static const int cache_size = N + 2 * max_blur_radius;

groupshared float4 g_sharedCache[cache_size];

[numthreads(N, 1, 1)]
void BlurFilter( uint3 DispatchThreadID : SV_DispatchThreadID,
					uint3 GroupThreadID : SV_GroupThreadID)
{
	
}