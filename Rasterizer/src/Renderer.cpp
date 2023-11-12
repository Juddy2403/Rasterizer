//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_AspectRatio = float(m_Width) / float(m_Height);
	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const std::vector<Vertex> vertices_world
		{ Vertex{Vector3{0.f,2.f,0.f}},
		Vertex{Vector3{1.f,0.f,0.f}},
		Vertex{Vector3{-1.f,0.f,0.f}} };
	std::vector<Vertex> vertices_ndc{};
	std::vector<Vector2> vertices_rasterized{};
	VertexTransformationFunction(vertices_world, vertices_ndc);
	NDCToRaster(vertices_ndc, vertices_rasterized);

	float xMin{ float(INT_MAX) }, xMax{ float(INT_MIN) }, yMin{ float(INT_MAX) }, yMax{ float(INT_MIN) };
	for (const Vector2& vertex : vertices_rasterized)
	{
		if (xMin > vertex.x) xMin = vertex.x;
		if (yMin > vertex.y) yMin = vertex.y;
		if (xMax < vertex.x) xMax = vertex.x;
		if (yMax < vertex.y) yMax = vertex.y;
	}
	boundingBox.push_back(BoundingBox{ xMin,xMax,yMin,yMax });

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			Vector2 P{ px + 0.5f,py + 0.5f };
			ColorRGB finalColor{};

			//Pixel color is black by default
			finalColor = colors::Black;

			//checking if the pixel is within the bounding box of the triangle
			if (boundingBox[0].IsPointInBox(P))
			{
				if (Utils::TriangleHitTest(vertices_rasterized[0], vertices_rasterized[1], vertices_rasterized[2], P))
					finalColor = colors::White;
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::NDCToRaster(const std::vector<Vertex>& vertices_in, std::vector<Vector2>& vertices_out) const
{
	vertices_out.reserve(vertices_in.size());
	for (const Vertex& vertex : vertices_in)
	{
		const float vertX{ (vertex.position.x + 1) / 2.f * m_Width };
		const float vertY{ (1 - vertex.position.y) / 2.f * m_Height };
		vertices_out.push_back(Vector2{ vertX,vertY });
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage

	vertices_out.reserve(vertices_in.size());
	for (size_t i = 0; i < vertices_in.size(); i++)
	{
		//copying all the vertex info into the new one
		vertices_out.push_back(vertices_in[i]);

		//pos to view space
		vertices_out[i].position = m_Camera.invViewMatrix.TransformPoint(vertices_in[i].position);

		//projecting view space
		vertices_out[i].position.x /= vertices_out[i].position.z;
		vertices_out[i].position.y /= vertices_out[i].position.z;

		//applying camera settings
		vertices_out[i].position.x /= (m_AspectRatio * m_Camera.fov);
		vertices_out[i].position.y /= m_Camera.fov;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
