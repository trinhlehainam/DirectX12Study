#include "common.hlsli"

cbuffer Matrix:register (b0)
{
	matrix world;
	matrix viewproj;
	//matrix mat;
}

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD)
{
	VsOutput ret;
	pos = mul(mul(viewproj,world), pos);
	ret.svpos = pos;
	ret.pos = pos;
	ret.uv = uv;
	return ret;
}