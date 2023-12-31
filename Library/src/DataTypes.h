#pragma once
//#include "Maths.h"
#include "vector"
#include "Texture.h"

namespace dae
{
	struct BoundingBox 
	{

		int xMin{};
		int yMin{};
		int xMax{};
		int yMax{};

		bool isOutOfScreen{};

		BoundingBox(int _xMin, int _xMax, int _yMin, int _yMax) : xMin{ _xMin }, xMax{ _xMax }, yMin{ _yMin }, yMax{ _yMax } {}
		//bool IsPointInBox(const Vector2& point)
		//{
		//	if (point.x < xMin) return false;
		//	if (point.x > xMax) return false;
		//	if (point.y < yMin) return false;
		//	if (point.y > yMax) return false;
		//	return true;
		//}
	};

	struct Vertex
	{
		Vector3 position{};
		Vector2 uv{}; //W2
		ColorRGB color{colors::White};
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		//Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};

		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		Texture* diffuseColor{};
		Texture* normalMap{};
		Texture* specularMap{};
		Texture* glossinessMap{};

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};

		float yaw{};
		Matrix translateMatrix{};
		Matrix rotationMatrix{};

		std::vector<BoundingBox> bounding_boxes{};

		void FreeTextures()
		{
			delete diffuseColor;
			delete normalMap;
			delete specularMap;
			delete glossinessMap;
		}
	};
}
