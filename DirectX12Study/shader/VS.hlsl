///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
};

/// Vertex shader
///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
VsOutput VS( float4 pos : POSITION )
{
	VsOutput ret;
	ret.svpos = pos;
	ret.pos = pos;
	return ret;
}