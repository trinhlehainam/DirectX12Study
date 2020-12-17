#pragma once

#include <stdint.h>
#include <vector>
#include <DirectXMath.h>

class GeometryGenerator
{
public:
	struct Vertex
	{
		Vertex() {};

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

	// Create cylinder parallel to y-axis, and center it at origin 
	// Adjust top and bottom Radius to create cone-liked cylinders or even cones
	// Adjust number of stack and slice to control detail of shape (the degree of tessellation)
	static Mesh CreateCylinder(float bottomRadius, float topRadius, float height, 
		uint32_t sliceCount, uint32_t stackCount);

	// Create sphere using spherical coordinate method (slice and stack)
	// Center point of sphere is place at origin with given radius
	// Adjust number of stack and slice to control detail of shape (the degree of tessellation)
	static Mesh CreateSphere(float radius, uint32_t stackCount, uint32_t sliceCount);

	/// <summary>
	/// Create Grid in x-z plane and centered it at origin
	/// </summary>
	/// <param name="width: ">length in X-axis</param>
	/// <param name="depth: ">length in Y-axis</param>
	/// <param name="num_grid_x: ">number of quads (cels) in X-axis</param>
	/// <param name="num_grid_z: ">number of quads (cels) in Z-axis</param>
	/// <returns>Mesh of created grid for rendering</returns>
	static Mesh CreateGrid(float width, float depth, uint32_t num_grid_x, uint32_t num_grid_z);

private:
	static void BuildCylinderTopCap(const float& topRadius, const float& height, const uint32_t& verticesPerRing, Mesh& mesh);
	static void BuildCylinderBottomCap(const float& bottomRadius, const float& height, const uint32_t& verticesPerRing, Mesh& mesh);
};

