#include "common.hlsli"

struct GSInput
{
	float3 center : POSITION;
	float2 size : SIZE;
};

struct GSOutput
{
	float4 svpos : SV_POSITION;
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

static float3 UP = { 0.0f, 1.0f, 0.0f };

[maxvertexcount(4)]
void TreeBillboardGS(
	point GSInput input[1],
	inout TriangleStream<GSOutput> output
)
{
	float3 look = g_viewPos - input[0].center;
	look.y = 0.0f;
	look = normalize(look);
	float3 right = cross(UP, look);
	float halfWidth = input[0].size.x / 2.0f;
	float halfHeight = input[0].size.y / 2.0f;
	
	float4 v[4];
	v[0] = float4(input[0].center - halfWidth * right - halfHeight * UP, 1.0f);
	v[1] = float4(input[0].center - halfWidth * right + halfHeight * UP, 1.0f);
	v[2] = float4(input[0].center + halfWidth * right - halfHeight * UP, 1.0f);
	v[3] = float4(input[0].center + halfWidth * right + halfHeight * UP, 1.0f);
	
	float2 uv[4] =
	{
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
	};
	
	GSOutput element;
	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		element.pos = v[i].xyz;
#if SHADOW_PIPELINE
		element.svpos = mul(g_lights[0].ProjectMatrix, v[i]);
#else
		element.svpos = mul(g_viewproj, v[i]);
#endif
		element.normal = look;
		element.uv = uv[i];
		
		output.Append(element);
	}
}