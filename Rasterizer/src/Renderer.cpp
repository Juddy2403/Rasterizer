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
	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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
	{   
	//Triangle 1
		Vertex{Vector3{0.f,2.f,0.f},ColorRGB{1,0,0}},
		Vertex{Vector3{1.5f,-1.f,0.f },ColorRGB{1,0,0}},
		Vertex{Vector3{-1.5f,-1.f,0.f },ColorRGB{1,0,0}}, 
	//Triangle 2
		Vertex{Vector3{0.f,4.f,2.f},ColorRGB{1,0,0}},
		Vertex{Vector3{3.f,-2.f,2.f },ColorRGB{0,1,0}},
		Vertex{Vector3{-3.f,-2.f,2.f },ColorRGB{0,0,1}}
	};
	std::vector<Vertex> vertices_ndc{};
	std::vector<Vertex> vertices_rasterized{};
	VertexTransformationFunction(vertices_world, vertices_ndc);
	NDCToRaster(vertices_ndc, vertices_rasterized);
	std::vector<BoundingBox> boundingBox{};
	TrianglesBoundingBox(vertices_rasterized, boundingBox);
	
	//Depth buffer
	std::fill(&m_pDepthBufferPixels[0], &m_pDepthBufferPixels[m_Width * m_Height-1], FLT_MAX);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			/*float gradient = px / static_cast<float>(m_Width);
			gradient += py / static_cast<float>(m_Width);
			gradient /= 2.0f;*/

			Vector2 P{ px + 0.5f,py + 0.5f };
			ColorRGB finalColor{}, interpolatedColor{};

			//Pixel color is black by default
			finalColor = colors::Black;

			for (size_t i = 0; i < vertices_rasterized.size(); i += 3)
			{
				//checking if the pixel is within the bounding box of the triangle
				if (boundingBox[i/3].IsPointInBox(P))
				{
					float pixelDepth{};
					if (Utils::TriangleHitTest(vertices_rasterized[i], vertices_rasterized[i+1], vertices_rasterized[i+2], P, interpolatedColor, pixelDepth))
					{
						if (m_pDepthBufferPixels[px + (py * m_Width)] > pixelDepth)
						{
							m_pDepthBufferPixels[px + (py * m_Width)] = pixelDepth;
							finalColor = interpolatedColor;
						}
					}
				}
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

void Renderer::NDCToRaster(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.reserve(vertices_in.size());
	for (size_t i = 0; i < vertices_in.size(); i++)
	{
		vertices_out.push_back(vertices_in[i]);
		const float vertX{ (vertices_in[i].position.x + 1) / 2.f * m_Width};
		const float vertY{ (1 - vertices_in[i].position.y) / 2.f * m_Height};
		vertices_out[i].position.x = vertX;
		vertices_out[i].position.y = vertY;
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

void Renderer::TrianglesBoundingBox(const std::vector<Vertex>& vertices, std::vector<BoundingBox>& bb) const
{
	for (size_t i = 0; i < vertices.size(); i+=3)
	{
		uint16_t xMin{ UINT16_MAX }, xMax{ 0 }, yMin{ UINT16_MAX }, yMax{ 0 }; 
		for (size_t j = 0; j < 3; j++)
		{
			if (xMin > vertices[i+j].position.x) xMin =  vertices[i+j].position.x;
			if (yMin > vertices[i+j].position.y) yMin =  vertices[i+j].position.y;
			if (xMax < vertices[i+j].position.x) xMax =  vertices[i+j].position.x;
			if (yMax < vertices[i+j].position.y) yMax =  vertices[i+j].position.y;
		}
		//xMin = std::max(int(xMin), 0);
		//yMin = std::max(int(yMin), 0);
		//xMax = std::min(int(xMax), m_Width - 1);
		//yMax = std::min(int(yMax), m_Height - 1);
		bb.push_back(BoundingBox{ xMin,xMax,yMin,yMax });
	}
	
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
