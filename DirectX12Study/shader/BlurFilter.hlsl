
cbuffer RootConstant : register(b0)
{
	int g_blurRadius;
	float g_w0;
	float g_w1;
	float g_w2;
	float g_w3;
	float g_w4;
	float g_w5;
	float g_w6;
	float g_w7;
	float g_w8;
	float g_w9;
	float g_w10;
}

Texture2D g_input : register(t0);
RWTexture2D<float4> g_output : register(u0);

#define N 256
static const int max_blur_radius = 5;
// expend cache size at start and end to R(g_blurRadius) respectively
// so the first and last threads of group can sample more pixels
// -> cacheSize = N + 2*R
static const int group_cache_size = N + 2 * max_blur_radius;

groupshared float4 g_sharedCache[group_cache_size];

[numthreads(N, 1, 1)]
void BlurFilter( uint3 DispatchThreadID : SV_DispatchThreadID,
					uint3 GroupThreadID : SV_GroupThreadID)
{
	float weights[] = { g_w0, g_w1, g_w2, g_w3, g_w4, g_w5, g_w6, g_w7, g_w8, g_w9, g_w10 };
	uint inputWidth = 0;
	uint inputHeight = 0;
	g_input.GetDimensions(inputWidth, inputHeight);
	//
	// Prevent access out-of-bounds index that causing unpredicted value
	//
	// Clamp x index of image at LEFT border
	uint leftLimit = max(DispatchThreadID.x - g_blurRadius, 0);
	// Clamp x index of image at RIGHT border
	uint rightLimit = min(DispatchThreadID.x + g_blurRadius, inputWidth);
	
	// First R(g_blurRadius) threads need to fetch R extra pixels
	// at the LEFT side of current sampling input texture's pixels
	// to the first R slots of shared cache
	if (GroupThreadID.x < g_blurRadius)
	{
		g_sharedCache[GroupThreadID.x] = g_input[uint2(leftLimit, DispatchThreadID.y)];
	}
	
	// Last R(N - g_blurRadius) threads need to fetch R extra pixels
	// at the RIGHT side of current sampling input texture's pixels
	// to the last R slots of shared cache
	if (GroupThreadID.x >= N - g_blurRadius)
	{
		g_sharedCache[GroupThreadID.x + 2 * g_blurRadius] = g_input[uint2(rightLimit, DispatchThreadID.y)];
	}
	
	// N threads sample texture normally
	// with base offset of group cache start from R (g_blurRadius + GroupThreadID.x)
	g_sharedCache[GroupThreadID.x + g_blurRadius] = g_input[uint2(rightLimit, DispatchThreadID.y)];
	
	GroupMemoryBarrierWithGroupSync();
	
	// Blur input texture and dispatch to output by sampling pixels at fetched cache
	
	float4 blur = 0.0f;
	for (int i = -g_blurRadius; i < g_blurRadius; ++i)
	{
		blur += weights[g_blurRadius + i] * g_sharedCache[g_blurRadius + GroupThreadID.x + i];
	}

	g_output[DispatchThreadID.xy] = blur;
}