#pragma once

#include <cstdint>
#include <vector>
#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;
struct BoundingBox;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{

	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;

	private:
		struct BoundingBox {

			float xMin{};
			float yMin{};
			float xMax{};
			float yMax{};

			BoundingBox(float _xMin, float _xMax, float _yMin, float _yMax) : xMin{ _xMin }, xMax{ _xMax }, yMin{ _yMin }, yMax{ _yMax } {}
			bool IsPointInBox(const Vector2& point)
			{
				if (point.x < xMin) return false;
				if (point.x > xMax) return false;
				if (point.y < yMin) return false;
				if (point.y > yMax) return false;
				return true;
			}
		};
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		//float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		

		const std::vector<Vector3> vertices_ndc
		{
			{0.f,.5f,1.f},
			{.5f,-.5f,1.f},
			{-.5f,-.5f,1.f}
		};
		std::vector<Vector2> vertices_rasterized{};
		std::vector<BoundingBox> boundingBox{};

		int m_Width{};
		int m_Height{};
	};
	

}
