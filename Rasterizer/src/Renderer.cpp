//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Texture.h"
#include "Utils.h"
//#include <algorithm>
//#include <execution>
//#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	//Define triangle list mesh
	meshes_world
	{
		Mesh{
				{
				/*Vertex{ Vector3{-3,3,-2}, Vector2{0,0}},
				Vertex{ Vector3{0,3,-2}, Vector2{0.5f,0}},
				Vertex{ Vector3{3,3,-2}, Vector2{1,0}},
				Vertex{ Vector3{-3,0,-2}, Vector2{0,0.5f}},
				Vertex{ Vector3{0,0,-2}, Vector2{0.5f,0.5f}},
				Vertex{ Vector3{3,0,-2}, Vector2{1,0.5f}},
				Vertex{ Vector3{-3,-3,-2}, Vector2{0,1}},
				Vertex{ Vector3{0,-3,-2}, Vector2{0.5f,1}},
				Vertex{ Vector3{3,-3,-2}, Vector2{1,1}},*/
				},
				{
					/*3,0,1,  1,4,3,  4,1,2,
					2,5,4,  6,3,4,  4,7,6,
					7,4,5,  5,8,7*/

					/*3,0,4,1,5,2,
					2,6,
					6,3,7,4,8,5*/
				},
			PrimitiveTopology::TriangleList
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
		meshes_world[0].diffuseColor = Texture::LoadFromFile("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/vehicle_diffuse.png");
		meshes_world[0].normalMap = Texture::LoadFromFile("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/vehicle_normal.png");
		meshes_world[0].specularMap = Texture::LoadFromFile("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/vehicle_specular.png");
		meshes_world[0].glossinessMap = Texture::LoadFromFile("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/vehicle_gloss.png");
	}
	catch (const FileNotFound& ex) {
		std::cout << "File not found \n";
	}

	Utils::ParseOBJ("D:/Howest/Sem 3/GP1-Rasterizer/Rasterizer/Rasterizer/Resources/vehicle.obj",
		meshes_world[0].vertices, meshes_world[0].indices);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(45.f, { .0f,0.0f,0.f }, float(m_Width) / float(m_Height));

	//Translate mesh
	meshes_world[0].worldMatrix = meshes_world[0].translateMatrix = Matrix::CreateTranslation(0, 0, 50);

}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;

	for (Mesh& mesh : meshes_world)
		mesh.FreeTextures();
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	// Update vertices
	for (Mesh& mesh : meshes_world)
	{
		VertexMatrixTransform(mesh);
		TrianglesBoundingBox(mesh);
	}

	if (m_IsRotating) RotateMesh(pTimer);

}

void dae::Renderer::RotateMesh(Timer* pTimer)
{
	meshes_world[0].yaw += pTimer->GetElapsed();
	meshes_world[0].rotationMatrix = Matrix::CreateRotationY(meshes_world[0].yaw);
	meshes_world[0].worldMatrix = meshes_world[0].rotationMatrix * meshes_world[0].translateMatrix;
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
			if (!mesh.bounding_boxes[i].isOutOfScreen)
#if defined(MULTI_THREADING)
				m_ImageHorizontalIterator.clear();
			m_ImageVerticalIterator.clear();
			m_ImageHorizontalIterator.resize(boundingBox.xMax);
			m_ImageVerticalIterator.resize(boundingBox.yMax);
			int iteratorIdx{ 0 };
			for (int idx = boundingBox.xMin; idx < boundingBox.xMax; ++idx)
				m_ImageHorizontalIterator[iteratorIdx++] = idx;
			iteratorIdx = 0;
			for (int idx = boundingBox.yMin; idx < boundingBox.yMax; ++idx)
				m_ImageVerticalIterator[iteratorIdx++] = idx;
			std::for_each(std::execution::par, m_ImageHorizontalIterator.begin(), m_ImageHorizontalIterator.end(), [&](uint32_t px)
				{
					std::for_each(std::execution::par, m_ImageVerticalIterator.begin(), m_ImageVerticalIterator.end(), [&](uint32_t py)
						{
							const Vector2 P(px + 0.5f, py + 0.5f);

							ColorRGB finalColor{};
							float pixelDepth{};
							Vector2 interpolatedUV{};

							const bool doesTriangleHit = (i % 2 == 0 || mesh.primitiveTopology == PrimitiveTopology::TriangleList) ?
								Utils::TriangleHitTest(mesh.vertices_out[index0], mesh.vertices_out[index1], mesh.vertices_out[index2], P, interpolatedUV, pixelDepth) :
								Utils::TriangleHitTest(mesh.vertices_out[index0], mesh.vertices_out[index2], mesh.vertices_out[index1], P, interpolatedUV, pixelDepth);

							const int bufferIndex = px + (py * m_Width);
							if (doesTriangleHit && m_pDepthBufferPixels[bufferIndex] > pixelDepth)
							{
								m_pDepthBufferPixels[bufferIndex] = pixelDepth;

								if (m_VisualMode == VisualMode::finalColor)  finalColor = m_pTexture->Sample(interpolatedUV);
								else
								{
									const float minInterpolation{ 0.985f }, maxInterpolation{ 1.f };
									Remap(pixelDepth, minInterpolation, maxInterpolation, 0.f, .9f);
									finalColor = ColorRGB{ pixelDepth,pixelDepth,pixelDepth };
								}

								// Update Color in Buffer
								finalColor.MaxToOne();
								m_pBackBufferPixels[bufferIndex] = SDL_MapRGB(m_pBackBuffer->format,
									static_cast<uint8_t>(finalColor.r * 255),
									static_cast<uint8_t>(finalColor.g * 255),
									static_cast<uint8_t>(finalColor.b * 255));
							}
						});
				});
#else
				for (int px = boundingBox.xMin; px < boundingBox.xMax; ++px)
				{
					for (int py = boundingBox.yMin; py < boundingBox.yMax; ++py)
					{
						const Vector2 P(px + 0.5f, py + 0.5f);

						const bool doesTriangleHit = (i % 2 == 0 || mesh.primitiveTopology == PrimitiveTopology::TriangleList) ?
							TriangleHitTest(mesh.vertices_out[index0], mesh.vertices_out[index1], mesh.vertices_out[index2], P) :
							TriangleHitTest(mesh.vertices_out[index0], mesh.vertices_out[index2], mesh.vertices_out[index1], P);

						Vertex_Out interpolatedVert{};
						if (doesTriangleHit)
						{	//Calculating the interpolated vert only if pixel is inside triangle
							interpolatedVert = (i % 2 == 0 || mesh.primitiveTopology == PrimitiveTopology::TriangleList) ?
								InterpolatedVertex(mesh.vertices_out[index0], mesh.vertices_out[index1], mesh.vertices_out[index2], P) :
								InterpolatedVertex(mesh.vertices_out[index0], mesh.vertices_out[index2], mesh.vertices_out[index1], P);

							const int bufferIndex = px + (py * m_Width);
							//Checking for frustrum culling and if vert is in front
							if (m_pDepthBufferPixels[bufferIndex] > interpolatedVert.position.z && interpolatedVert.position.z > 0 && interpolatedVert.position.z < 1)
							{
								m_pDepthBufferPixels[bufferIndex] = interpolatedVert.position.z;
								ColorRGB finalColor = PixelShading(interpolatedVert);

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
#endif
		}
	}



	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);

}

void Renderer::ToggleVisualMode()
{
	m_VisualMode = ShadingMode((static_cast<int>(m_VisualMode) + 1) % 4);
	switch (m_VisualMode)
	{
	case ShadingMode::observedArea:
		std::cout << "Shading mode: obeserved area \n";
		break;
	case ShadingMode::diffuse:
		std::cout << "Shading mode: diffuse \n";
		break;
	case ShadingMode::specular:
		std::cout << "Shading mode: specular \n";
		break;
	case ShadingMode::combined:
		std::cout << "Shading mode: combined \n";
		break;
	default:
		break;
	}
}

ColorRGB Renderer::PixelShading(const Vertex_Out& v)
{
	Vector3 lightDirection = { .577f, -.577f, .577f };
	const float lightIntensity{ 7.f };
	const ColorRGB ambientOcclusion = { 0.025f, 0.025f,0.025f };

	Vector3 normal{ v.normal };

	if (m_IsNormalMapToggled)
	{
		ColorRGB normalMapSample{ meshes_world[0].normalMap->Sample(v.uv) };
		// Remapping the value from [0,1] to [-1,1] (i am already remapping from [0,255] in the Sample function
		normalMapSample = 2.f * normalMapSample - ColorRGB{ 1.f,1.f,1.f };

		Vector3 sampledNormal{ Vector3{normalMapSample.r,normalMapSample.g,normalMapSample.b} };

		// Transforming it in the correct tangent space
		const Vector3 binormal{ Vector3::Cross(v.normal,v.tangent) };
		Matrix tangentSpaceTransfMatrix{ v.tangent, binormal, v.normal,Vector3{0,0,0} };
		sampledNormal = tangentSpaceTransfMatrix.TransformVector(sampledNormal);
		normal = sampledNormal;
	}

	const float lightDirCos = Vector3::Dot(normal, -lightDirection);

	if (lightDirCos >= 0)
		switch (m_VisualMode)
		{
		case ShadingMode::observedArea:
		{
			return ColorRGB{ lightDirCos,lightDirCos,lightDirCos };
			break;
		}
		case ShadingMode::diffuse:
		{
			const ColorRGB cd{ meshes_world[0].diffuseColor->Sample(v.uv) };
			const ColorRGB BRDF{ Utils::Lambert( cd) };
			return BRDF * lightDirCos * lightIntensity;
			break;
		}

		case ShadingMode::specular:
		{
			// Sampling color from maps
			const float glossinesMapSample{ meshes_world[0].glossinessMap->Sample(v.uv).r };
			const float specularMapSample{ meshes_world[0].specularMap->Sample(v.uv).r };
			const float shininess{ 25.f };
			// SpecularColor sampled from SpecularMap and PhongExponent from GlossinessMap
			const ColorRGB specular{ Utils::Phong(specularMapSample, glossinesMapSample * shininess, lightDirection, -v.viewDirection, normal) };
			return specular * lightDirCos;
			break;
		}
		case ShadingMode::combined:
		{
			// Sampling color from maps
			const float glossinesMapSample{ meshes_world[0].glossinessMap->Sample(v.uv).r };
			const float specularMapSample{ meshes_world[0].specularMap->Sample(v.uv).r };
			const float shininess{ 25.f };
			// SpecularColor sampled from SpecularMap and PhongExponent from GlossinessMap
			const ColorRGB specular{ Utils::Phong(specularMapSample, glossinesMapSample * shininess, lightDirection, -v.viewDirection, normal) };
			const ColorRGB cd{ meshes_world[0].diffuseColor->Sample(v.uv) };
			const ColorRGB BRDF{ Utils::Lambert( cd) };
			return ColorRGB{ lightDirCos  * (specular + BRDF * lightIntensity + ambientOcclusion) };
			break;
		}
		//case ShadingMode::depthBuffer:
		//{
		//	const float minInterpolation{ 0.985f }, maxInterpolation{ 1.f };
		//	float remappedDepthValue{ m_pDepthBufferPixels[bufferIdx] };
		//	Remap(remappedDepthValue, minInterpolation, maxInterpolation, 0.f, .9f);
		//	finalColor = ColorRGB{ remappedDepthValue,remappedDepthValue,remappedDepthValue };
		//}
		//break;
		}
	return ColorRGB{};

}

inline void Renderer::VertexMatrixTransform(Mesh& mesh) const noexcept
{
	//Projection Stage
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(mesh.vertices.size());

	//Getting the final transform matrix
	const Matrix worldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.invViewMatrix * m_Camera.projectionMatrix };

	for (size_t i = 0; i < mesh.vertices.size(); i++)
	{
		mesh.vertices_out.push_back(Vertex_Out{ Vector4{mesh.vertices[i].position,1}, mesh.vertices[i].color,mesh.vertices[i].uv
			,mesh.vertices[i].normal,mesh.vertices[i].tangent });
		mesh.vertices_out[i].position = worldViewProjectionMatrix.TransformPoint(mesh.vertices_out[i].position);

		//normals need to be multiplied with the world matrix only & no perspective divide for them
		mesh.vertices_out[i].normal = mesh.worldMatrix.TransformVector(mesh.vertices_out[i].normal);

		//same for tangents
		mesh.vertices_out[i].tangent = mesh.worldMatrix.TransformVector(mesh.vertices_out[i].tangent);

		//NDC transformation/ Perspective divide
		mesh.vertices_out[i].position.x /= mesh.vertices_out[i].position.w;
		mesh.vertices_out[i].position.y /= mesh.vertices_out[i].position.w;
		mesh.vertices_out[i].position.z /= mesh.vertices_out[i].position.w;

		const float vertX{ (mesh.vertices_out[i].position.x + 1) / 2.f * m_Width };
		const float vertY{ (1 - mesh.vertices_out[i].position.y) / 2.f * m_Height };
		mesh.vertices_out[i].position.x = vertX;
		mesh.vertices_out[i].position.y = vertY;

	}
}

inline void Renderer::TrianglesBoundingBox(Mesh& mesh) const noexcept
{//should be calculated inside the mesh struct instead
	mesh.bounding_boxes.clear();
	const size_t numTriangles = (mesh.primitiveTopology == PrimitiveTopology::TriangleList) ? mesh.indices.size() / 3 : mesh.indices.size() - 2;
	mesh.bounding_boxes.reserve(numTriangles);

	for (size_t i = 0; i < numTriangles; ++i)
	{
		int minX, maxX, minY, maxY;
		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			minX = static_cast<int>(std::min(mesh.vertices_out[mesh.indices[i * 3]].position.x, std::min(mesh.vertices_out[mesh.indices[i * 3 + 1]].position.x, mesh.vertices_out[mesh.indices[i * 3 + 2]].position.x)) - 0.5f);
			maxX = static_cast<int>(std::max(mesh.vertices_out[mesh.indices[i * 3]].position.x, std::max(mesh.vertices_out[mesh.indices[i * 3 + 1]].position.x, mesh.vertices_out[mesh.indices[i * 3 + 2]].position.x)) + 0.5f);
			minY = static_cast<int>(std::min(mesh.vertices_out[mesh.indices[i * 3]].position.y, std::min(mesh.vertices_out[mesh.indices[i * 3 + 1]].position.y, mesh.vertices_out[mesh.indices[i * 3 + 2]].position.y)) - 0.5f);
			maxY = static_cast<int>(std::max(mesh.vertices_out[mesh.indices[i * 3]].position.y, std::max(mesh.vertices_out[mesh.indices[i * 3 + 1]].position.y, mesh.vertices_out[mesh.indices[i * 3 + 2]].position.y)) + 0.5f);
			break;
		case PrimitiveTopology::TriangleStrip:
			minX = static_cast<int>(std::min(mesh.vertices_out[mesh.indices[i]].position.x, std::min(mesh.vertices_out[mesh.indices[i + 1]].position.x, mesh.vertices_out[mesh.indices[i + 2]].position.x)) - 0.5f);
			maxX = static_cast<int>(std::max(mesh.vertices_out[mesh.indices[i]].position.x, std::max(mesh.vertices_out[mesh.indices[i + 1]].position.x, mesh.vertices_out[mesh.indices[i + 2]].position.x)) + 0.5f);
			minY = static_cast<int>(std::min(mesh.vertices_out[mesh.indices[i]].position.y, std::min(mesh.vertices_out[mesh.indices[i + 1]].position.y, mesh.vertices_out[mesh.indices[i + 2]].position.y)) - 0.5f);
			maxY = static_cast<int>(std::max(mesh.vertices_out[mesh.indices[i]].position.y, std::max(mesh.vertices_out[mesh.indices[i + 1]].position.y, mesh.vertices_out[mesh.indices[i + 2]].position.y)) + 0.5f);
			break;
		}
		bool isOutOfScreen{ false };
		if (minX < 0 || minY < 0 || maxX > m_Width || maxY > m_Height)
			isOutOfScreen = true;
		// if statement of std::clamp from C++ 20
		Clamp(minX, 0, m_Width);
		Clamp(maxX, 0, m_Width);
		Clamp(minY, 0, m_Height);
		Clamp(maxY, 0, m_Height);
		mesh.bounding_boxes.push_back(BoundingBox{ minX,maxX,minY,maxY });
		mesh.bounding_boxes[i].isOutOfScreen = isOutOfScreen;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::ToggleRotation()
{
	m_IsRotating = !m_IsRotating;
}

void dae::Renderer::ToggleNormals()
{
	m_IsNormalMapToggled = !m_IsNormalMapToggled;
}

inline bool Renderer::TriangleHitTest(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Vector2& pixelVector) noexcept
{
	// Calculate vectors and cross products
	const Vector2 edge0 = v1.position - v0.position;
	const Vector2 edge1 = v2.position - v1.position;
	const Vector2 edge2 = v0.position - v2.position;

	const Vector2 vertToPixel0 = pixelVector - v0.position;
	const Vector2 vertToPixel1 = pixelVector - v1.position;
	const Vector2 vertToPixel2 = pixelVector - v2.position;

	m_Cross0 = Vector2::Cross(edge0, vertToPixel0);
	m_Cross1 = Vector2::Cross(edge1, vertToPixel1);
	// Check if the pixel is inside the triangle
	if (m_Cross0 * m_Cross1 < 0) return false;
	m_Cross2 = Vector2::Cross(edge2, vertToPixel2);

	// Check if the pixel is inside the triangle
	if (m_Cross0 * m_Cross2 < 0) return false;

	return true;
}

inline Vertex_Out& Renderer::InterpolatedVertex(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const Vector2& pixelVector)
{
	Vertex_Out interpolatedVert{};
	const Vector2 edge0 = v1.position - v0.position;
	const Vector2 edge1 = v2.position - v1.position;

	// Calculate barycentric coordinates
	const float doubleArea = Vector2::Cross(edge0, edge1);
	const float W2 = m_Cross0 / doubleArea;
	const float W0 = m_Cross1 / doubleArea;
	const float W1 = m_Cross2 / doubleArea;

	// Calculating pixel depth
	const float recipZ0 = 1.0f / v0.position.z;
	const float recipZ1 = 1.0f / v1.position.z;
	const float recipZ2 = 1.0f / v2.position.z;

	interpolatedVert.position.z = 1.0f / (W0 * recipZ0 + W1 * recipZ1 + W2 * recipZ2);

	// Calculate interpolated values 
	const float recipW0 = 1.0f / v0.position.w;
	const float recipW1 = 1.0f / v1.position.w;
	const float recipW2 = 1.0f / v2.position.w;

	interpolatedVert.position.w = 1.0f / (W0 * recipW0 + W1 * recipW1 + W2 * recipW2);

	// Calculate interpolated texCoord
	interpolatedVert.uv = (v0.uv * W0 * recipW0 + v1.uv * W1 * recipW1 + v2.uv * W2 * recipW2) * interpolatedVert.position.w;

	// Calculate interpolated normal
	interpolatedVert.normal = (v0.normal * W0 * recipW0 + v1.normal * W1 * recipW1 + v2.normal * W2 * recipW2) * interpolatedVert.position.w;
	interpolatedVert.normal.Normalize();

	// Calculate interpolated color
	interpolatedVert.color = (v0.color * W0 * recipW0 + v1.color * W1 * recipW1 + v2.color * W2 * recipW2) * interpolatedVert.position.w;

	// Calculate interpolated tangent
	interpolatedVert.tangent = (v0.tangent * W0 * recipW0 + v1.tangent * W1 * recipW1 + v2.tangent * W2 * recipW2) * interpolatedVert.position.w;

	// Calculate interpolated view direction
	interpolatedVert.viewDirection = Vector3{ m_Camera.origin ,Vector3{interpolatedVert.position} };

	return interpolatedVert;
}