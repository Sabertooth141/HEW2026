#include "BitmapFont.h"

#include "Renderer.h"

#include <algorithm>
#include <cassert>

#pragma comment(lib, "gdi32.lib")

namespace
{
	// UTF-32 -> UTF-16, since that is what GDI speaks. Returns the unit count.
	int ToUtf16(char32_t cp, wchar_t out[2])
	{
		if (cp < 0x10000)
		{
			out[0] = static_cast<wchar_t>(cp);
			return 1;
		}

		const char32_t v = cp - 0x10000;
		out[0] = static_cast<wchar_t>(0xD800 + (v >> 10));
		out[1] = static_cast<wchar_t>(0xDC00 + (v & 0x3FF));
		return 2;
	}

	bool IsBlank(char32_t cp)
	{
		return cp == U' ' || cp == U'\u3000'; // ascii space, ideographic space
	}
}

BitmapFont::BitmapFont(Renderer& renderer, const wchar_t* fontName, int pixelHeight, int inAtlasSize)
	: atlasSize(inAtlasSize)
{
	HDC screenDC = GetDC(nullptr);
	dc = CreateCompatibleDC(screenDC);
	ReleaseDC(nullptr, screenDC);

	gdiFont = CreateFontW(
		-pixelHeight, 0, 0, 0,
		FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, // let the face supply whatever script it has
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY, // greyscale AA - ClearType would give coloured fringes
		FIXED_PITCH | FF_MODERN,
		fontName);

	oldFont = SelectObject(dc, gdiFont);

	TEXTMETRICW tm = {};
	GetTextMetricsW(dc, &tm);

	lineHeight = tm.tmHeight;

	// half-width reference, used as the fallback advance for missing glyphs
	SIZE half = {};
	GetTextExtentPoint32W(dc, L"A", 1, &half);
	halfAdvance = half.cx;

	// a cell has to hold the widest glyph in the face, which for a Japanese
	// font is a full-width one. +1 px of padding keeps filtering from bleeding.
	SIZE fullSize = {};
	GetTextExtentPoint32W(dc, L"\u3042", 1, &fullSize); // HIRAGANA A
	cellWidth = std::max({ static_cast<int>(tm.tmMaxCharWidth), static_cast<int>(fullSize.cx), halfAdvance * 2 }) + 1;
	cellHeight = tm.tmHeight + 1;

	columns = atlasSize / cellWidth;
	rows = atlasSize / cellHeight;
	assert(columns > 0 && rows > 0 && "BitmapFont: atlas too small for one cell");

	// one-cell scratch DIB that every glyph is drawn into before upload
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = cellWidth;
	bmi.bmiHeader.biHeight = -cellHeight; // negative -> top-down, matches texture row order
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
	assert(dib && dibBits && "BitmapFont: CreateDIBSection failed");

	oldBmp = SelectObject(dc, dib);

	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));

	scratch.resize(static_cast<size_t>(cellWidth) * static_cast<size_t>(cellHeight));

	// empty atlas, filled in cell by cell as glyphs are asked for
	const std::vector<uint32_t> blank(static_cast<size_t>(atlasSize) * static_cast<size_t>(atlasSize), 0u);

	D3D11_TEXTURE2D_DESC td = {};
	td.Width = static_cast<UINT>(atlasSize);
	td.Height = static_cast<UINT>(atlasSize);
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT; // DEFAULT so UpdateSubresource can patch single cells
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = blank.data();
	sd.SysMemPitch = static_cast<UINT>(atlasSize) * 4;

	HRESULT hr = renderer.GetDevice()->CreateTexture2D(&td, &sd, &atlas);
	assert(SUCCEEDED(hr) && "BitmapFont: CreateTexture2D failed");

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
	srvd.Format = td.Format;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MostDetailedMip = 0;
	srvd.Texture2D.MipLevels = 1;

	hr = renderer.GetDevice()->CreateShaderResourceView(atlas.Get(), &srvd, &srv);
	assert(SUCCEEDED(hr) && "BitmapFont: CreateShaderResourceView failed");

	(void)hr;
}

BitmapFont::~BitmapFont()
{
	if (dc)
	{
		SelectObject(dc, oldBmp);
		SelectObject(dc, oldFont);
		DeleteDC(dc);
	}

	if (dib) DeleteObject(dib);
	if (gdiFont) DeleteObject(gdiFont);
}

const Glyph* BitmapFont::GetGlyph(Renderer& renderer, char32_t codepoint)
{
	const auto it = glyphs.find(codepoint);
	if (it != glyphs.end())
	{
		return &it->second;
	}

	Glyph glyph;
	if (!Rasterize(renderer, codepoint, glyph))
	{
		return nullptr;
	}

	const auto [pos, inserted] = glyphs.emplace(codepoint, glyph);
	return &pos->second;
}

bool BitmapFont::Rasterize(Renderer& renderer, char32_t codepoint, Glyph& out)
{
	wchar_t utf16[2] = {};
	const int units = ToUtf16(codepoint, utf16);

	SIZE extent = {};
	GetTextExtentPoint32W(dc, utf16, units, &extent);
	out.advance = static_cast<float>(extent.cx);

	// whitespace only moves the pen, so it never costs a cell
	if (IsBlank(codepoint))
	{
		out.width = 0.f;
		out.height = 0.f;
		return true;
	}

	if (nextCell >= columns * rows)
	{
		assert(false && "BitmapFont: glyph atlas full - raise atlasSize");
		return false;
	}

	// draw into the scratch cell
	RECT cell{ 0, 0, cellWidth, cellHeight };
	FillRect(dc, &cell, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
	TextOutW(dc, 0, 0, utf16, units);
	GdiFlush();

	// GDI wrote white-on-black with no alpha, so move the coverage into alpha
	const uint8_t* src = static_cast<const uint8_t*>(dibBits);
	for (size_t i = 0; i < scratch.size(); ++i)
	{
		const uint8_t b = src[i * 4 + 0];
		const uint8_t g = src[i * 4 + 1];
		const uint8_t r = src[i * 4 + 2];
		const uint32_t alpha = (std::max)({ r, g, b });

		scratch[i] = 0x00FFFFFFu | (alpha << 24); // R8G8B8A8_UNORM is 0xAABBGGRR in memory
	}

	const int cellX = (nextCell % columns) * cellWidth;
	const int cellY = (nextCell / columns) * cellHeight;
	++nextCell;

	D3D11_BOX box = {};
	box.left = static_cast<UINT>(cellX);
	box.top = static_cast<UINT>(cellY);
	box.front = 0;
	box.right = static_cast<UINT>(cellX + cellWidth);
	box.bottom = static_cast<UINT>(cellY + cellHeight);
	box.back = 1;

	renderer.GetContext()->UpdateSubresource(atlas.Get(), 0, &box, scratch.data(),
		static_cast<UINT>(cellWidth) * 4, 0);

	const float size = static_cast<float>(atlasSize);
	out.uvMin = { static_cast<float>(cellX) / size, static_cast<float>(cellY) / size };
	out.uvMax = {
		static_cast<float>(cellX + cellWidth) / size,
		static_cast<float>(cellY + cellHeight) / size
	};
	out.width = static_cast<float>(cellWidth);
	out.height = static_cast<float>(cellHeight);

	return true;
}