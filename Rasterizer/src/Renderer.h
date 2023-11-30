#pragma once

#include <cstdint>
#include <vector>
#include "Camera.h"

//#define MULTI_THREADING

struct SDL_Window;
struct SDL_Surface;
struct BoundingBox;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex_Out;
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
		void ToggleVisualMode();
		bool SaveBufferToImage() const;
		void ToggleRotation();
		void ToggleNormals();
		void ToggleDepthBuffer();
		

	private:
		
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBufferPixels{};

		enum class ShadingMode {
			observedArea,
			diffuse,
			specular,
			combined
		};
		ShadingMode m_VisualMode{ ShadingMode::combined };
		
		std::vector<Mesh> meshes_world;
#if defined(MULTI_THREADING)
		std::vector<uint32_t> m_ImageHorizontalIterator, m_ImageVerticalIterator;
#endif
		Camera m_Camera{};

		int m_Width{};
		int m_Height{};
		float m_Cross0{}, m_Cross1{}, m_Cross2{};
		bool m_IsRotating{ true };
		bool m_IsNormalMapToggled{ true };
		bool m_IsDepthBufferToggled{ false };

		//void VertexTransformationFunction( std::vector<Mesh>& meshes) const;
		inline void VertexMatrixTransform(Mesh& mesh) const noexcept;
		inline void TrianglesBoundingBox(Mesh& mesh) const noexcept;
		//void NDCToRaster(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		inline bool TriangleHitTest(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Vector2& pixelVector) noexcept;
		inline Vertex_Out& InterpolatedVertex(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Vector2& pixelVector);
		ColorRGB PixelShading(const Vertex_Out& v);
		void RotateMesh(Timer* pTimer);
	};
	

}
