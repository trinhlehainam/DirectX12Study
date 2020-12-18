cbuffer worldPass:register (b0)
{
	float4x4 g_viewproj; // projecting object to window space matrix
	float4x4 g_lightViewProj;
	float3 g_viewPos;
	float padding0;
	float3 g_lightPos;
	float padding1;
}



