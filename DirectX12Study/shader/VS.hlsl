#include "common.hlsli"

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( float4 pos : POSITION, float2 uv : TEXCOORD)
{
	VsOutput ret;
	ret.svpos = pos;
	ret.pos = pos;
	ret.uv = uv;
	return ret;
}