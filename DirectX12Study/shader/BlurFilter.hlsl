
cbuffer RootConstant : register(b0)
{
	int g_blurRadius;
	float g_weights[11];
}

Texture2D g_input : register(t0);
RWTexture2D<float4> g_output : register(u0);

#define N 256
static const int max_blur_radius = 5;
static const int cache_size = N + 2 * max_blur_radius;

groupshared float4 g_sharedCache[N];

[numthreads(N, 1, 1)]
void BlurFilter( uint3 DispatchThreadID : SV_DispatchThreadID,
					uint3 GroupThreadID : SV_GroupThreadID)
{
	g_output[DispatchThreadID.xy] = g_input[DispatchThreadID.xy];

	GroupMemoryBarrierWithGroupSync();
	
	g_output[DispatchThreadID.xy] = g_sharedCache[GroupThreadID.x];

}