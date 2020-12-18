#pragma once
#include <DirectXMath.h>
#include <vector>

namespace Geometry
{
	struct Vertex
	{
		Vertex();

		Vertex(const DirectX::XMFLOAT3& position,
			const DirectX::XMFLOAT3& normal,
			const DirectX::XMFLOAT3& tangentU,
			const DirectX::XMFLOAT2& uv);

		Vertex(float positionX, float positionY, float positionZ,
			float normalX, float normalY, float normalZ,
			float tangentX, float tangentY, float tangentZ,
			float u, float v);

		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		// Tangent Unit (Unit length)
		DirectX::XMFLOAT3 tangentU;
		DirectX::XMFLOAT2 texCoord;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;
	};
}

