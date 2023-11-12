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

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	for (Vector3 vertex : vertices_ndc)
	{
		vertices_rasterized.push_back(Vector2{ Utils::RasterSpaceX(vertex.x, m_Width), Utils::RasterSpaceY(vertex.y, m_Height) });
	}

	float xMin{ float(INT_MAX) }, xMax{ float(INT_MIN) }, yMin{ float(INT_MAX) }, yMax{ float(INT_MIN) };
	for (Vector2 vertex : vertices_rasterized)
	{
		if (xMin > vertex.x) xMin = vertex.x;
		if (yMin > vertex.y) yMin = vertex.y;
		if (xMax < vertex.x) xMax = vertex.x;
		if (yMax < vertex.y) yMax = vertex.y;
	}
	boundingBox.push_back(BoundingBox{ xMin,xMax,yMin,yMax });
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
	
	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			Vector2 P{ px + 0.5f,py + 0.5f};
			ColorRGB finalColor{};

			finalColor = colors::Black;
			if(boundingBox[0].IsPointInBox(P))
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


void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
