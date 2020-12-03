cbuffer Matrix:register (b0)
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
};

PrimitiveOut primitiveVS(float4 pos : POSITION)
{
	PrimitiveOut ret;
	ret.pos = mul(world, pos);
	ret.svpos = mul(viewproj, ret.pos);
	ret.lvpos = mul(lightViewProj, ret.pos);
	return ret;
}