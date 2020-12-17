struct PrimitiveOut
{
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
	float4 lvpos : POSITION1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};