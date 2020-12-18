#include "GeometryGenerator.h"

using namespace DirectX;

Geometry::Mesh GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius,
	float height, uint32_t sliceCount, uint32_t stackCount)
{
	Geometry::Mesh mesh;

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

		// raidus of ring
		float r = bottomRadius + i*deltaRadius;

		// angle in ring surface
		float theta = XM_2PI / vertices_per_ring;

		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			Geometry::Vertex vertex;

			float c = cosf(j * theta);	// sin
			float s = sinf(j * theta);	// cos

			/*-------------WATCH LATER*-------------*/
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
			/*--------------------------------------*/

			// Force move sematic to avoid copy
			mesh.vertices.push_back(std::move(vertex));
		}
	}

	// number of quad in one ring
	float quadCount = vertices_per_ring + 1;

	constexpr float indices_per_quad = 6;
	const float num_indices = indices_per_quad * quadCount * stackCount;
	mesh.indices.reserve(num_indices);

	// Count indices per quad (2 triangles)
	// Number of collumn of quad are equal to stack count
	const uint32_t& num_quad_collumn = stackCount;
	for (uint32_t i = 0; i < num_quad_collumn; ++i)
	{
		// Index of vertex in one ring
		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			// Index of next vertex
			// When next index is larger than last index of the ring
			// It moves back to start index of the ring
			uint32_t nextIndex = (j+1) % vertices_per_ring;

			// B ********* C  i + 1
			//	 *     * *		^
			//	 *   *   *		|
			//	 * *	 *		|
			// A ********* D	i
			//	 j ----> j + 1 (next vertex index)
			
			// ABC
			mesh.indices.push_back(i * vertices_per_ring + j);
			mesh.indices.push_back((i + 1) * vertices_per_ring + j);
			mesh.indices.push_back((i + 1) * vertices_per_ring + nextIndex);

			// ACD
			mesh.indices.push_back(i * vertices_per_ring + j);
			mesh.indices.push_back((i + 1) * vertices_per_ring + nextIndex);
			mesh.indices.push_back(i * vertices_per_ring + nextIndex);
		}
	}
	BuildCylinderTopCap(topRadius, height, vertices_per_ring, mesh);
	BuildCylinderBottomCap(bottomRadius, height, vertices_per_ring, mesh);

	return mesh;
}

Geometry::Mesh GeometryGenerator::CreateSphere(float radius, uint32_t stackCount, uint32_t sliceCount)
{
	Geometry::Mesh mesh;

	//
	/* COMPUTE VERTICES */
	//

	// angle step to offset value of each step when ring moves
	const float deltaTheta = XM_PI / stackCount;
	// angle step to offset vertex around the ring
	const float deltaPhi = XM_2PI / sliceCount;

	// Top and bottom poles don't count as ring
	// ( stackCount - 1 ) -> number of rings in QUAD stacks
	// ( -2 )             -> Remove stacks of top and bottom poles
	const uint32_t ring_count = stackCount + 1 - 2;
	const uint32_t& vertices_per_ring = sliceCount;

	for (uint32_t i = 0; i < ring_count; ++i)
	{
		// The angle of first ring equal to angle of the second stack
		// Offset the angle index of ring to angle index of stack
		// => ( i + 1 )
		float theta = (i+1) * deltaTheta;

		// y position of ring
		float y = radius * cosf(theta);

		// start point (first vertex) on the ring in z-x coordinate
		// use this position to rotate round y-axis in z-x coordinate
		// to create another vertex on the ring
		float startX = radius * sinf(theta);

		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			Geometry::Vertex vertex;
			float phi = j * deltaPhi;

			float c = cosf(phi);
			float s = sinf(phi);
			
			vertex.position = { startX * c , y , startX * s };

			/*-------------WATCH LATER*-------------*/
			vertex.tangentU = { -startX * s , 0.0f, +startX * c };

			XMVECTOR T = XMLoadFloat3(&vertex.tangentU);
			XMStoreFloat3(&vertex.tangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&vertex.position);
			XMStoreFloat3(&vertex.normal, XMVector3Normalize(p));

			vertex.texCoord.x = theta / XM_2PI;
			vertex.texCoord.y = phi / XM_PI;
			/*--------------------------------------*/

			// Force move sematics to avoid copy
			mesh.vertices.push_back(std::move(vertex));
		}
	}

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Geometry::Vertex topVertex(0.0f, +radius, 0.0f,
		0.0f, +1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);
	Geometry::Vertex bottomVertex(0.0f, -radius, 0.0f,
		0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	// add top north vertex first to mesh
	mesh.vertices.push_back(std::move(topVertex));
	const uint32_t top_vertex_index = mesh.vertices.size() - 1;

	// Add bottom south vertex
	mesh.vertices.push_back(std::move(bottomVertex));
	const uint32_t bottom_vertex_index = mesh.vertices.size() - 1;

	//
	/* COMPUTE INDICES */
	//

	/* Add indices for vertices connect to TOP pole */

	for (uint32_t i = 0; i < vertices_per_ring; ++i)
	{
		uint32_t nextIndex = (i + 1) % vertices_per_ring;
		mesh.indices.push_back(top_vertex_index);
		mesh.indices.push_back(nextIndex);
		mesh.indices.push_back(i);
	}

	/* Add indices for all quads ( two triangles ) */

	const uint32_t num_quad_collumn = stackCount - 2;
	for (uint32_t i = 0; i < num_quad_collumn; ++i)
	{
		for (uint32_t j = 0; j < vertices_per_ring; ++j)
		{
			// Index of next vertex
			// When next index is larger than last index of the ring
			// It moves back to start index of the ring
			uint32_t nextIndex = (j + 1) % vertices_per_ring;

			// B ********* C    i
			//	 *     * *	  	|
			//	 *   *   *	  	|
			//	 * *	 *	  	V
			// A ********* D  i + 1
			//	 j ----> j + 1 (next vertex index)

			// ABC
			mesh.indices.push_back((i + 1) * vertices_per_ring + j);
			mesh.indices.push_back(i * vertices_per_ring + j);
			mesh.indices.push_back(i * vertices_per_ring + nextIndex);

			// ACD
			mesh.indices.push_back((i + 1) * vertices_per_ring + j);
			mesh.indices.push_back(i * vertices_per_ring + nextIndex);
			mesh.indices.push_back((i + 1) * vertices_per_ring + nextIndex);
		}
	}

	/* Add indices for vertices connect to BOTTOM pole */

	// Index of first vertex on last ring
	const uint32_t last_ring_start_vertex_index = top_vertex_index - vertices_per_ring;
	for (uint32_t i = 0; i < vertices_per_ring; ++i)
	{
		uint32_t nextIndex = (i + 1) % vertices_per_ring;
		mesh.indices.push_back(bottom_vertex_index);
		mesh.indices.push_back(last_ring_start_vertex_index + i);
		mesh.indices.push_back(last_ring_start_vertex_index + nextIndex);
	}

	return mesh;
}

Geometry::Mesh GeometryGenerator::CreateGrid(float width, float depth, uint32_t num_grid_x, uint32_t num_grid_z)
{
	Geometry::Mesh mesh;

	const uint32_t num_vertices_x = num_grid_x + 1;
	const uint32_t num_vertices_z = num_grid_z + 1;

	mesh.vertices.reserve(num_vertices_x * num_vertices_z);
	uint32_t faceCount = (num_grid_x) * (num_grid_z) * 2; // number of triangles

	const float quad_width = width / num_grid_x;
	const float quad_depth = depth / num_grid_z;

	const float half_width = width / 2.0f;
	const float half_depth = depth / 2.0f;

	float du = 1.0f / num_vertices_x;
	float dv = 1.0f / num_vertices_z;

	for (int i = 0; i < num_vertices_x; ++i)
	{
		float posX = half_width - i * quad_width;
		for (int j = 0; j < num_vertices_z; ++j)
		{
			Geometry::Vertex vertex;

			vertex.position = { posX, 0.0f, half_depth - j * quad_depth };
			vertex.normal = { 0.0f,1.0f,0.0f };
			vertex.texCoord = { i * du, j * dv };
			vertex.tangentU = { 1.0f,0.0f,0.0f };

			mesh.vertices.push_back(std::move(vertex));
		}
	}

	const uint32_t vertices_per_quad = 6;
	mesh.indices.reserve(faceCount * vertices_per_quad);

	for (int i = 0; i < num_grid_x; ++i)
	{
		for (int j = 0; j < num_grid_z; ++j)
		{
			mesh.indices.push_back((i + 1) * num_vertices_z + j);
			mesh.indices.push_back(i * num_vertices_z + j);
			mesh.indices.push_back(i * num_vertices_z + j + 1);

			mesh.indices.push_back((i + 1) * num_vertices_z + j);
			mesh.indices.push_back(i * num_vertices_z + j + 1);
			mesh.indices.push_back((i + 1) * num_vertices_z + j + 1);
		}
	}

	return mesh;
}

void GeometryGenerator::BuildCylinderTopCap(const float& topRadius, const float& height,
	const uint32_t& verticesPerRing, Geometry::Mesh& mesh)
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

		mesh.vertices.push_back(Geometry::Vertex(
			x, y, z, 
			0.0f, 1.0f, 0.0f, 
			1.0f, 0.0f, 0.0f, 
			u, v));
	}

	// Vertex of cap center
	mesh.vertices.push_back(Geometry::Vertex(
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
	const uint32_t& verticesPerRing, Geometry::Mesh& mesh)
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

		mesh.vertices.push_back(Geometry::Vertex(
			x, y, z,
			0, 1.f, 0,
			1.f, 0, 0,
			u, v));
	}

	// Vertex of cap center
	mesh.vertices.push_back(Geometry::Vertex(
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

