cbuffer passConstant:register (b0)
{
	matrix world;			// transform to world space matrix
	matrix viewproj;		// projecting object to window space matrix
	vector lightPos;
	matrix shadow;
	matrix lightViewProj;
}

struct PrimitiveOut
{
	float4 svpos: SV_POSITION;
	float4 pos: POSITION;
	float4 lvpos : POSITION1;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

struct PrimitiveInput
{
	float4 pos : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

PrimitiveOut primitiveVS(PrimitiveInput input)
{
	PrimitiveOut ret;
	ret.pos = mul(world, input.pos);
	
	// Remove translate
	matrix wrl = world;
	wrl._14_24_34 = 0.0f;
	ret.normal = mul(wrl, input.normal);
	ret.tangent = input.tangent;
	ret.uv = input.uv;
	ret.svpos = mul(viewproj, ret.pos);
	ret.lvpos = mul(lightViewProj, ret.pos);
	return ret;
}