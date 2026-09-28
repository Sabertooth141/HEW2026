#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION

#include "TextureCache.h"
#include "stb_image.h"

std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> TextureCache::cache;

namespace
{
	std::string ToUtf8(const std::wstring& w)
	{
		if (w.empty()) return {};
		const int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
			nullptr, 0, nullptr, nullptr);
		std::string out(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
			out.data(), len, nullptr, nullptr);
		return out;
	}

	struct StbiBuffer
	{
		void* data = nullptr;
		~StbiBuffer() { if (data) stbi_image_free(data); }
	};
}

ID3D11ShaderResourceView* TextureCache::Load(Renderer& renderer, const std::wstring& path)
{
	auto it = cache.find(path);
	if (it != cache.end())
	{
		return it->second.Get();
	}

	const std::string narrowPath = ToUtf8(path);

	int width, height, channels;
	StbiBuffer pixels;
	DXGI_FORMAT fmt;
	UINT bytesPerPixel;

	if (stbi_is_hdr(narrowPath.c_str()))
	{
		// --- HDR / float ---
		pixels.data = stbi_loadf(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
		bytesPerPixel = 16;
	}
	else if (stbi_is_16_bit(narrowPath.c_str()))
	{
		// --- 16-bit ---
		pixels.data = stbi_load_16(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R16G16B16A16_UNORM;
		bytesPerPixel = 8;
	}
	else
	{
		// --- 8-bit (default) ---
		pixels.data = stbi_load(narrowPath.c_str(), &width, &height, &channels, 4);
		fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
		bytesPerPixel = 4;
	}

	assert(pixels.data && "TextureCache: stbi_load failed");
	if (!pixels.data)
	{
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = static_cast<UINT>(width);
	td.Height = static_cast<UINT>(height);
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = fmt;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = pixels.data;
	sd.SysMemPitch = static_cast<UINT>(width) * bytesPerPixel;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
	HRESULT hr = GetDevice(renderer)->CreateTexture2D(&td, &sd, &pTexture);
	assert(SUCCEEDED(hr) && "TextureCache: CreateTexture2D failed");
	if (FAILED(hr))
	{
		return nullptr;
	}

	// create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = fmt;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = 1;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	hr = GetDevice(renderer)->CreateShaderResourceView(pTexture.Get(), &srvd, &srv);
	assert(SUCCEEDED(hr) && "TextureCache: CreateShaderResourceView failed");
	if (FAILED(hr))
	{
		return nullptr;
	}

	auto [pos, inserted] = cache.emplace(path, std::move(srv));
	return pos->second.Get();
}

void TextureCache::Clear()
{
	cache.clear();
}

void TextureCache::Bind(Renderer& renderer)
{
	return;
}

ID3D11ShaderResourceView* TextureCache::LoadSolid(Renderer& renderer, uint32_t rgba)
{
	// Cache key per color so repeated calls reuse the same SRV.
	const std::wstring key = L"__solid_" + std::to_wstring(rgba);

	auto it = cache.find(key);
	if (it != cache.end())
	{
		return it->second.Get();
	}

	// One RGBA pixel. Byte order matches DXGI_FORMAT_R8G8B8A8_UNORM: R,G,B,A.
	const uint32_t pixel = rgba;

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = 1;
	td.Height = 1;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = &pixel;
	sd.SysMemPitch = 4; // 1 pixel * 4 bytes

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
	HRESULT hr = GetDevice(renderer)->CreateTexture2D(&td, &sd, &tex);
	assert(SUCCEEDED(hr) && "TextureCache: solid CreateTexture2D failed");
	if (FAILED(hr))
	{
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = td.Format;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = 1;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	hr = GetDevice(renderer)->CreateShaderResourceView(tex.Get(), &srvd, &srv);
	assert(SUCCEEDED(hr) && "TextureCache: solid CreateShaderResourceView failed");
	if (FAILED(hr))
	{
		return nullptr;
	}

	auto [pos, inserted] = cache.emplace(key, std::move(srv));
	return pos->second.Get();
}