#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>



namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		Texture* texture{ new Texture{IMG_Load(path.c_str()) } };
		if (texture->m_pSurface == nullptr) throw FileNotFound();
		return texture;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct texel for the given uv
		Uint8 r{}, g{}, b{};
		int uIdx{ static_cast<int>(uv.x * m_pSurface->w) }, vIdx{ static_cast<int>(uv.y * m_pSurface->h) };

		// Ensure indices are within bounds
		uIdx = std::clamp(uIdx, 0, m_pSurface->w - 1);
		vIdx = std::clamp(vIdx, 0, m_pSurface->h - 1);

		SDL_GetRGB(m_pSurfacePixels[uIdx + (vIdx * m_pSurface->h)], m_pSurface->format, &r, &g, &b);
		ColorRGB color{ (r / 255.f),(g / 255.f) ,(b / 255.f) };
		return color;
	}
}