#include "GeometryCommon.h"

Geometry::Vertex::Vertex()
{
}

Geometry::Vertex::Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal,
	const DirectX::XMFLOAT3& tangentU, const DirectX::XMFLOAT2& uv)
	:position(position), normal(normal), tangentU(tangentU), texCoord(uv)
{
}

Geometry::Vertex::Vertex(float positionX, float positionY, float positionZ,
	float normalX, float normalY, float normalZ,
	float tangentX, float tangentY, float tangentZ,
	float u, float v)
	: position(positionX, positionY, positionZ),
	normal(normalX, normalY, normalZ),
	tangentU(tangentX, tangentY, tangentZ),
	texCoord(u, v)
{
}