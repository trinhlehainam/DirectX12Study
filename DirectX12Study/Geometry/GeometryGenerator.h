#pragma once

#include <stdint.h>

#include "GeometryCommon.h"

namespace GeometryGenerator
{
	// Create cylinder parallel to y-axis, and center it at origin 
	// Adjust top and bottom Radius to create cone-liked cylinders or even cones
	// Adjust number of stack and slice to control detail of shape (the degree of tessellation)
	Geometry::Mesh CreateCylinder(float bottomRadius, float topRadius, float height, 
		uint32_t sliceCount, uint32_t stackCount);

	// Create sphere using spherical coordinate method (slice and stack)
	// Center point of sphere is place at origin with given radius
	// Adjust number of stack and slice to control detail of shape (the degree of tessellation)
	Geometry::Mesh CreateSphere(float radius, uint32_t stackCount, uint32_t sliceCount);

	/// <summary>
	/// Create Grid in x-z plane and centered it at origin
	/// </summary>
	/// <param name="width: ">length in X-axis</param>
	/// <param name="depth: ">length in Z-axis</param>
	/// <param name="num_grid_x: ">number of quads (cels) in X-axis</param>
	/// <param name="num_grid_z: ">number of quads (cels) in Z-axis</param>
	/// <returns>Mesh of created grid for rendering</returns>
	Geometry::Mesh CreateGrid(float width, float depth, uint32_t num_grid_x, uint32_t num_grid_z);

	/// <summary>
	/// Create Box with center at origin
	/// </summary>
	/// <param name="width: ">length in X-axis</param>
	/// <param name="height: ">length in Y-axis</param>
	/// <param name="depth: ">length in Z-axis</param>
	/// <returns></returns>
	Geometry::Mesh CreateBox(float width, float height, float depth);
};

