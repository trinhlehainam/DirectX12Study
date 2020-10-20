///基本頂点シェーダ
/// @param pos 頂点座標
/// @return システム頂点座標
float4 VS( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}