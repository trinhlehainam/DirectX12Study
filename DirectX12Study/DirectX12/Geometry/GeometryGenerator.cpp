#include "GeometryGenerator.h"

using namespace DirectX;

GeometryGenerator::Vertex::Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, 
	const DirectX::XMFLOAT3& tangentU, const DirectX::XMFLOAT2& uv):
	position(position),normal(normal),tangentU(tangentU),texCoord(uv)
{
}

GeometryGenerator::Vertex::Vertex(float positionX, float positionY, float positionZ, 
	float normalX, float normalY, float normalZ, 
	float tangentX, float tangentY, float tangentZ, 
	float u, float v):
	position(positionX,positionY,positionZ),
	normal(normalX,normalY,normalZ),
	tangentU(tangentX, tangentY, tangentZ),
	texCoord(u,v)
{
}

GeometryGenerator::Mesh GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, 
	float height, uint32_t stackCount, uint32_t sliceCount)
{
	Mesh mesh;

	const uint32_t& vertices_per_ring = sliceCount;

	uint32_t ringCount = stackCount + 1;
	/*-Vairables use to offset ring-*/
	// The amount of height that ring move up to one step
	float stackHeight = height / stackCount;

	// The amount of value that radius increase when move up to one stack
	// Radius increase from BOTTOM to TOP
	float deltaRadius = (topRadius - bottomRadius) / stackCount;
	/*-------------------------------*/

	// Number of vertices include :
	// The number of vertices at center of rings			(number of rings)
	// The number of vertices around center of each ring	(number of slice) * (number of rings)
	const uint32_t num_vertices = ringCount * (vertices_per_ring + 1);
	// Reserve vector for performance
	mesh.vertices.reserve(num_vertices);

	for (uint32_t i = 0; i < ringCount; ++i)
	{
		// Place center of shape at origin
		// Minus half height of shape to move center of shape to origin
		float y = -0.5f * height + i*stackHeight;

		float r = bottomRadius + i*deltaRadius;

		float theta = XM_2PI / vertices_per_ring;
		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			Vertex vertex;

			float c = cosf(j * theta);
			float s = sinf(j * theta);

			// Watch later
			vertex.texCoord.x = (float)j / vertices_per_ring;
			vertex.texCoord.y = 1.0f - (float)i / stackCount;
			vertex.tangentU = { -s,0.0f,c };

			vertex.position = { r * c,y,r * s };

			float dr = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dr * c, -height, dr * s);

			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.normal, N);

			mesh.vertices.push_back(std::move(vertex));
		}
	}

	// number of quad in one ring
	float quadCount = vertices_per_ring + 1;

	constexpr float indices_per_quad = 6;
	const float num_indices = indices_per_quad * quadCount * stackCount;
	mesh.indices.reserve(num_indices);

	// Count indices per quad (2 triangles)
	// Number of collumn quad are equal to stack count
	for (uint32_t i = 0; i < stackCount; ++i)
	{
		// Index of vertex in one ring
		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			// Index of next vertex
			uint32_t nextVertexIndex = (j+1) % vertices_per_ring;

			// B ********* C  i + 1
			//	 *     * *
			//	 *   *   *
			//	 * *	 *
			// A ********* D	i
			//	 j       j+1 (next vertex index)
			
			// ABC
			mesh.indices.push_back(i * vertices_per_ring + j);
			mesh.indices.push_back((i + 1) * vertices_per_ring + nextVertexIndex);
			mesh.indices.push_back((i+1) * vertices_per_ring + nextVertexIndex);

			// ACD
			mesh.indices.push_back(i * vertices_per_ring + j);
			mesh.indices.push_back((i + 1) * vertices_per_ring + nextVertexIndex);
			mesh.indices.push_back(i * vertices_per_ring + nextVertexIndex);
		}
	}
	BuildCylinderTopCap(topRadius, height, vertices_per_ring, mesh);
	BuildCylinderBottomCap(bottomRadius, height, vertices_per_ring, mesh);

	return mesh;
}

void GeometryGenerator::BuildCylinderTopCap(const float& topRadius, const float& height, 
	const uint32_t& verticesPerRing, Mesh& mesh)
{
	auto baseIndex = mesh.vertices.size();

	float y = 0.5f * height;
	float theta = XM_2PI / verticesPerRing;
	const XMFLOAT3 top_cap_normal = { 0,1.f,0 };

	for (uint32_t i = 0; i < verticesPerRing; ++i)
	{
		float x = topRadius * cosf(i*theta);
		float z = topRadius * sinf(i*theta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mesh.vertices.push_back(Vertex(
			x, y, z, 
			0.0f, 1.0f, 0.0f, 
			1.0f, 0.0f, 0.0f, 
			u, v));
	}

	// Vertex of cap center
	mesh.vertices.push_back(Vertex(
		0.0f, y, 0.0f,
		0.0f, 1.0f, 0.0f,
		1.f, 0.0f, 0.0f,
		0.5f, 0.5f));

	// Save index of vertex in center of cap
	auto centerIndex = mesh.vertices.size() - 1;
	
	// Top cap facing top
	// Count indices in clockwise
	for (uint32_t i = 0; i < verticesPerRing; ++i)
	{
		uint32_t nextIndex = (i + 1) % verticesPerRing;
		mesh.indices.push_back(centerIndex);
		mesh.indices.push_back(baseIndex + nextIndex);
		mesh.indices.push_back(baseIndex + i);
	}
}

void GeometryGenerator::BuildCylinderBottomCap(const float& bottomRadius, const float& height, 
	const uint32_t& verticesPerRing, Mesh& mesh)
{
	auto baseIndex = mesh.vertices.size();

	float y = -0.5f * height ;
	float theta = XM_2PI / verticesPerRing;
	const XMFLOAT3 bottom_cap_normal = { 0,-1.f,0 };

	for (uint32_t i = 0; i < verticesPerRing; ++i)
	{
		float x = bottomRadius * cosf(i * theta);
		float z = bottomRadius * sinf(i * theta);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		mesh.vertices.push_back(Vertex(
			x, y, z,
			0, 1.f, 0,
			1.f, 0, 0,
			u, v));
	}

	// Vertex of cap center
	mesh.vertices.push_back(Vertex(
		0, y, 0,
		0, 1.f, 0,
		1.f, 0, 0,
		0.5f, 0.5f));

	// Save index of vertex in center of cap
	auto centerIndex = mesh.vertices.size() - 1;

	// Bottom cap facing bottom
	// Count indices in COUNTER clockwise
	for (uint32_t i = 0; i < verticesPerRing; ++i)
	{
		uint32_t nextIndex = (i + 1) % verticesPerRing;
		mesh.indices.push_back(centerIndex);
		mesh.indices.push_back(baseIndex + i);
		mesh.indices.push_back(baseIndex + nextIndex);
	}
}
