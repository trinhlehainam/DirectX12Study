
cbuffer RootConstant : register(b0)
{
	int g_blurRadius;
	float w0;
	float w1;
	float w2;
	float w3;
	float w4;
	float w5;
	float w6;
	float w7;
	float w8;
	float w9;
	float w10;
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
	float weights[] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
	
	float sum = 0.0f;
	for (int i = 0; i < 11; ++i)
	{
		sum += weights[i];
	}
	g_output[DispatchThreadID.xy] = sum;
		
}