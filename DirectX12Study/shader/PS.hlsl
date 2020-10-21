///頂点シェーダ出力
struct VsOutput
{
	float4 svpos:SV_POSITION;  //	System value position
	float4 pos:POSITION;
};

// Pixel Shader
float4 PS(VsOutput input) : SV_TARGET
{
	// Return Object`s color
	// Convert -1 ~ 1 -> 0 ~ 1
	// Solution 
	// (-1 ~ 1) + 1 -> (0 ~ 2)
	// (0 ~ 2) / 2 -> (0 ~ 1)
	return (input.pos + 1.0f) / 2.0f;
}