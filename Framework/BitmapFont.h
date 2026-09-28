#pragma once
#include "Win.h"

#include <cstdint>
#include <d3d11.h>
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

class Renderer;

struct Glyph
{
	DirectX::XMFLOAT2 uvMin{ 0.f, 0.f };
	DirectX::XMFLOAT2 uvMax{ 0.f, 0.f };
	float width = 0.f;   // quad size in pixels (0 = nothing to draw, e.g. a space)
	float height = 0.f;
	float advance = 0.f; // pen step: half-width for ASCII, full-width for CJK
};

// Glyph atlas with a lazy cache: a codepoint is rasterized by GDI the first
// time it is drawn, parked in a free cell of the atlas texture, and looked up
// from the map on every frame after that. That keeps kanji workable - you only
// pay for the characters the game actually shows.
//
// The atlas is white RGB with the glyph coverage in alpha, so the vertex colour
// tints the text.
//
// fontName must be a face that has Japanese glyphs: "MS Gothic" (true monospace,
// half/full width exactly 1:2), "BIZ UDGothic", "Meiryo", "Yu Gothic UI".
class BitmapFont
{
public:
	BitmapFont(Renderer& renderer, const wchar_t* fontName = L"MS Gothic", int pixelHeight = 18,
		int atlasSize = 1024);
	~BitmapFont();

	BitmapFont(const BitmapFont&) = delete;
	BitmapFont& operator=(const BitmapFont&) = delete;

	// nullptr only when the atlas is full
	const Glyph* GetGlyph(Renderer& renderer, char32_t codepoint);

	ID3D11ShaderResourceView* GetSrv() const { return srv.Get(); }

	float GetLineHeight() const { return static_cast<float>(lineHeight); }
	float GetHalfAdvance() const { return static_cast<float>(halfAdvance); }

	int GetCachedGlyphCount() const { return static_cast<int>(glyphs.size()); }
	int GetCellCapacity() const { return columns * rows; }

private:
	bool Rasterize(Renderer& renderer, char32_t codepoint, Glyph& out);

private:
	std::unordered_map<char32_t, Glyph> glyphs;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> atlas;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

	// GDI scratch kept alive for on-demand rasterization
	HDC dc = nullptr;
	HFONT gdiFont = nullptr;
	HBITMAP dib = nullptr;
	void* dibBits = nullptr;
	HGDIOBJ oldFont = nullptr;
	HGDIOBJ oldBmp = nullptr;

	std::vector<uint32_t> scratch;

	int cellWidth = 0;  // sized for the widest (full-width) glyph
	int cellHeight = 0;
	int lineHeight = 0;
	int halfAdvance = 0;

	int atlasSize = 0;
	int columns = 0;
	int rows = 0;
	int nextCell = 0;
};