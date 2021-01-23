
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

groupshared float4 g_sharedCache[cache_size];

[numthreads(N, 1, 1)]
void BlurFilter( uint3 DispatchThreadID : SV_DispatchThreadID,
					uint3 GroupThreadID : SV_GroupThreadID)
{
	
	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
	if (GroupThreadID.x < g_blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(DispatchThreadID.x - g_blurRadius, 0);
		g_sharedCache[GroupThreadID.x] = g_input[int2(x, DispatchThreadID.y)];
	}
	if (GroupThreadID.x >= N - g_blurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(DispatchThreadID.x + g_blurRadius, g_input.Length.x - 1);
		g_sharedCache[GroupThreadID.x + 2 * g_blurRadius] = g_input[int2(x, GroupThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	g_sharedCache[GroupThreadID.x + g_blurRadius] = g_input[min(DispatchThreadID.xy, g_input.Length.xy - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

	float4 blurColor = 0.0f;
	
	for (int i = -g_blurRadius; i <= g_blurRadius; ++i)
	{
		int k = GroupThreadID.x + g_blurRadius + i;
		
		blurColor += g_weights[i + g_blurRadius] * g_sharedCache[k];
	}
	
	g_output[DispatchThreadID.xy] = blurColor;
		
}