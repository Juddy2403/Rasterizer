//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include <algorithm>
#include <execution>
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	//Define triangle list mesh
	meshes_world
	{
		Mesh{
				{
				Vertex{ Vector3{-3,3,-2}, Vector2{0,0}},
				Vertex{ Vector3{0,3,-2}, Vector2{0.5f,0}},
				Vertex{ Vector3{3,3,-2}, Vector2{1,0}},
				Vertex{ Vector3{-3,0,-2}, Vector2{0,0.5f}},
				Vertex{ Vector3{0,0,-2}, Vector2{0.5f,0.5f}},
				Vertex{ Vector3{3,0,-2}, Vector2{1,0.5f}},
				Vertex{ Vector3{-3,-3,-2}, Vector2{0,1}},
				Vertex{ Vector3{0,-3,-2}, Vector2{0.5f,1}},
				Vertex{ Vector3{3,-3,-2}, Vector2{1,1}},
				},
				{
					/*3,0,1,  1,4,3,  4,1,2,
					2,5,4,  6,3,4,  4,7,6,
					7,4,5,  5,8,7*/
					3,0,4,1,5,2,
					2,6,
					6,3,7,4,8,5
				},
			PrimitiveTopology::TriangleStrip
			}
	//Mesh{
	//		{
	//		//Triangle 1
	//			Vertex{Vector3{0.f,2.f,0.f},ColorRGB{1,0,0}},
	//			Vertex{Vector3{1.5f,-1.f,0.f },ColorRGB{1,0,0}},
	//			Vertex{Vector3{-1.5f,-1.f,0.f },ColorRGB{1,0,0}},
	//			//Triangle 2
	//				Vertex{Vector3{0.f,4.f,2.f},ColorRGB{1,0,0}},
	//				Vertex{Vector3{3.f,-2.f,2.f },ColorRGB{0,1,0}},
	//				Vertex{Vector3{-3.f,-2.f,2.f },ColorRGB{0,0,1}}
	//			},
	//			{
	//				0,1,2,  3,4,5
	//			},
	//		PrimitiveTopology::TriangleList
	//		}
	}
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	try
	{
		//loading texture
		m_pTexture = Texture::LoadFromFile("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/uv_grid_2.png");
	}
	catch (const FileNotFound& ex) {
		std::cout << "File not found \n";
	}

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_AspectRatio = float(m_Width) / float(m_Height);
	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });



#if defined(MULTI_THREADING)
	//Multi-threading 
	m_ImageHorizontalIterator.resize(m_Width);
	m_ImageVerticalIterator.resize(m_Height);
	for (int index = 0; index < m_Width; ++index)
		m_ImageHorizontalIterator[index] = index;
	for (int index = 0; index < m_Height; ++index)
		m_ImageVerticalIterator[index] = index;
#endif
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
	VertexTransformationFunction(meshes_world);
	TrianglesBoundingBox(meshes_world);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//Depth buffer
	std::fill(&m_pDepthBufferPixels[0], &m_pDepthBufferPixels[m_Width * m_Height - 1], FLT_MAX);

	//color the screen
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format,
		static_cast<uint8_t>(100.f * 255),
		static_cast<uint8_t>(100.f * 255),
		static_cast<uint8_t>(100.f * 255))
	);

	//RENDER LOGIC
#if defined(MULTI_THREADING)
	std::for_each(std::execution::par, m_ImageHorizontalIterator.begin(), m_ImageHorizontalIterator.end(), [&](uint32_t px)
		{
			std::for_each(std::execution::par, m_ImageVerticalIterator.begin(), m_ImageVerticalIterator.end(), [&](uint32_t py)
				{
					Vector2 P{ px + 0.5f,py + 0.5f };

					for (size_t i = 0; i < vertices_rasterized.size(); i += 3)
					{
						//checking if the pixel is within the bounding box of the triangle
						if (boundingBox[i / 3].IsPointInBox(P))
						{
							ColorRGB finalColor{}, interpolatedColor{};
							float pixelDepth{};
							if (Utils::TriangleHitTest(vertices_rasterized[i], vertices_rasterized[i + 1], vertices_rasterized[i + 2], P, interpolatedColor, pixelDepth))
							{
								if (m_pDepthBufferPixels[px + (py * m_Width)] > pixelDepth)
								{
									m_pDepthBufferPixels[px + (py * m_Width)] = pixelDepth;
									finalColor = interpolatedColor;

									//Update Color in Buffer
									finalColor.MaxToOne();
									m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
										static_cast<uint8_t>(finalColor.r * 255),
										static_cast<uint8_t>(finalColor.g * 255),
										static_cast<uint8_t>(finalColor.b * 255));
								}
							}
						}
					}


				});
		});
#else
for (Mesh& mesh : meshes_world)
{
    const size_t numTriangles = (mesh.primitiveTopology == PrimitiveTopology::TriangleList) ? mesh.indices.size() / 3 : mesh.indices.size() - 2;

    for (size_t i = 0; i < numTriangles; ++i)
    {
        const size_t baseIndex = (mesh.primitiveTopology == PrimitiveTopology::TriangleList) ? i * 3 : i;
        const size_t index0 = mesh.indices[baseIndex];
        const size_t index1 = mesh.indices[baseIndex + 1];
        const size_t index2 = mesh.indices[baseIndex + 2];

        const BoundingBox& boundingBox = mesh.bounding_boxes[i];

        for (int px = boundingBox.xMin; px < boundingBox.xMax; ++px)
        {
            for (int py = boundingBox.yMin; py < boundingBox.yMax; ++py)
            {
                const Vector2 P(px + 0.5f, py + 0.5f);

                ColorRGB finalColor;
                float pixelDepth;
                Vector2 interpolatedUV{};

                const bool doesTriangleHit = (i % 2 == 0 || mesh.primitiveTopology == PrimitiveTopology::TriangleList) ?
                    Utils::TriangleHitTest(mesh.transformed_vertices[index0], mesh.transformed_vertices[index1], mesh.transformed_vertices[index2], P, interpolatedUV, pixelDepth) :
                    Utils::TriangleHitTest(mesh.transformed_vertices[index0], mesh.transformed_vertices[index2], mesh.transformed_vertices[index1], P, interpolatedUV, pixelDepth);

                const int bufferIndex = px + (py * m_Width);
                if (doesTriangleHit && m_pDepthBufferPixels[bufferIndex] > pixelDepth)
                {
                    m_pDepthBufferPixels[bufferIndex] = pixelDepth;
                    finalColor = m_pTexture->Sample(interpolatedUV);

                    // Update Color in Buffer
                    finalColor.MaxToOne();
                    m_pBackBufferPixels[bufferIndex] = SDL_MapRGB(m_pBackBuffer->format,
                        static_cast<uint8_t>(finalColor.r * 255),
                        static_cast<uint8_t>(finalColor.g * 255),
                        static_cast<uint8_t>(finalColor.b * 255));
                }
            }
        }
    }
}


#endif
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
		const float vertX{ (vertices_in[i].position.x + 1) / 2.f * m_Width };
		const float vertY{ (1 - vertices_in[i].position.y) / 2.f * m_Height };
		vertices_out[i].position.x = vertX;
		vertices_out[i].position.y = vertY;
	}
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes) const
{
	//Projection Stage
	for (Mesh& mesh : meshes)
	{
		mesh.transformed_vertices.reserve(mesh.vertices.size());

		for (size_t i = 0; i < mesh.vertices.size(); i++)
		{
			mesh.transformed_vertices.push_back(mesh.vertices[i]);

			//pos to view space
			mesh.transformed_vertices[i].position = m_Camera.invViewMatrix.TransformPoint(mesh.vertices[i].position);

			//projecting view space
			mesh.transformed_vertices[i].position.x /= mesh.transformed_vertices[i].position.z;
			mesh.transformed_vertices[i].position.y /= mesh.transformed_vertices[i].position.z;

			//applying camera settings
			mesh.transformed_vertices[i].position.x /= (m_AspectRatio * m_Camera.fov);
			mesh.transformed_vertices[i].position.y /= m_Camera.fov;

			//NDC transformation
			const float vertX{ (mesh.transformed_vertices[i].position.x + 1) / 2.f * m_Width };
			const float vertY{ (1 - mesh.transformed_vertices[i].position.y) / 2.f * m_Height };
			mesh.transformed_vertices[i].position.x = vertX;
			mesh.transformed_vertices[i].position.y = vertY;
		}
	}
}

void Renderer::TrianglesBoundingBox(std::vector<Mesh>& meshes) const
{//should be calculated inside the mesh struct instead
	for (Mesh& mesh : meshes)
	{
		mesh.bounding_boxes.clear();
		const size_t numTriangles = (mesh.primitiveTopology == PrimitiveTopology::TriangleList) ? mesh.indices.size() / 3 : mesh.indices.size() - 2;
		mesh.bounding_boxes.reserve(numTriangles);

		for (size_t i = 0; i < numTriangles; ++i)
		{
			int minX, maxX, minY, maxY;
			if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
			{
				minX = static_cast<int>(std::min(mesh.transformed_vertices[mesh.indices[i * 3]].position.x, std::min(mesh.transformed_vertices[mesh.indices[i * 3 + 1]].position.x, mesh.transformed_vertices[mesh.indices[i * 3 + 2]].position.x)) - 0.5f);
				maxX = static_cast<int>(std::max(mesh.transformed_vertices[mesh.indices[i * 3]].position.x, std::max(mesh.transformed_vertices[mesh.indices[i * 3 + 1]].position.x, mesh.transformed_vertices[mesh.indices[i * 3 + 2]].position.x)) + 0.5f);
				minY = static_cast<int>(std::min(mesh.transformed_vertices[mesh.indices[i * 3]].position.y, std::min(mesh.transformed_vertices[mesh.indices[i * 3 + 1]].position.y, mesh.transformed_vertices[mesh.indices[i * 3 + 2]].position.y)) - 0.5f);
				maxY = static_cast<int>(std::max(mesh.transformed_vertices[mesh.indices[i * 3]].position.y, std::max(mesh.transformed_vertices[mesh.indices[i * 3 + 1]].position.y, mesh.transformed_vertices[mesh.indices[i * 3 + 2]].position.y)) + 0.5f);
			}
			else
			{
				minX = static_cast<int>(std::min(mesh.transformed_vertices[mesh.indices[i]].position.x, std::min(mesh.transformed_vertices[mesh.indices[i + 1]].position.x, mesh.transformed_vertices[mesh.indices[i + 2]].position.x)) - 0.5f);
				maxX = static_cast<int>(std::max(mesh.transformed_vertices[mesh.indices[i]].position.x, std::max(mesh.transformed_vertices[mesh.indices[i + 1]].position.x, mesh.transformed_vertices[mesh.indices[i + 2]].position.x)) + 0.5f);
				minY = static_cast<int>(std::min(mesh.transformed_vertices[mesh.indices[i]].position.y, std::min(mesh.transformed_vertices[mesh.indices[i + 1]].position.y, mesh.transformed_vertices[mesh.indices[i + 2]].position.y)) - 0.5f);
				maxY = static_cast<int>(std::max(mesh.transformed_vertices[mesh.indices[i]].position.y, std::max(mesh.transformed_vertices[mesh.indices[i + 1]].position.y, mesh.transformed_vertices[mesh.indices[i + 2]].position.y)) + 0.5f);
			}

			// if statement of std::clamp from C++ 20
			minX = std::ranges::clamp(minX, 0, m_Width);
			maxX = std::ranges::clamp(maxX, 0, m_Width);
			minY = std::ranges::clamp(minY, 0, m_Height);
			maxY = std::ranges::clamp(maxY, 0, m_Height);
			mesh.bounding_boxes.push_back(BoundingBox{ minX,maxX,minY,maxY });
		}

	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
