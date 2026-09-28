#include "TextRenderer.h"

#include <cstdarg>
#include <cstdio>

#include "InputLayout.h"
#include "PixelShader.h"
#include "Renderer.h"
#include "VertexShader.h"
#include "WindowSettings.h"

namespace
{
	struct TextTransforms
	{
		DirectX::XMMATRIX model;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	};

	constexpr size_t formatBufferSize = 512;

	// Decodes one UTF-8 sequence and moves the cursor past it.
	// Malformed bytes become U+FFFD instead of desynchronising the whole string.
	char32_t NextCodepoint(std::string_view text, size_t& i)
	{
		const auto byte = [&](size_t k) { return static_cast<unsigned char>(text[k]); };

		const unsigned char lead = byte(i);
		if (lead < 0x80)
		{
			++i;
			return lead;
		}

		int extra = 0;
		char32_t cp = 0;

		if ((lead & 0xE0) == 0xC0) { cp = lead & 0x1Fu; extra = 1; }
		else if ((lead & 0xF0) == 0xE0) { cp = lead & 0x0Fu; extra = 2; }
		else if ((lead & 0xF8) == 0xF0) { cp = lead & 0x07u; extra = 3; }
		else { ++i; return 0xFFFD; }

		if (i + static_cast<size_t>(extra) >= text.size())
		{
			i = text.size();
			return 0xFFFD;
		}

		for (int k = 1; k <= extra; ++k)
		{
			const unsigned char cont = byte(i + static_cast<size_t>(k));
			if ((cont & 0xC0) != 0x80)
			{
				++i;
				return 0xFFFD;
			}
			cp = (cp << 6) | (cont & 0x3Fu);
		}

		i += static_cast<size_t>(extra) + 1;
		return cp;
	}
}

TextRenderer::TextRenderer(Renderer& inRenderer, const wchar_t* fontName, int fontPixelHeight)
	: renderer(inRenderer)
	, screenWidth(static_cast<float>(WIN_WIDTH))
	, screenHeight(static_cast<float>(WIN_HEIGHT))
{
	font = std::make_unique<BitmapFont>(inRenderer, fontName, fontPixelHeight);

	vertexShader = std::make_unique<VertexShader>(inRenderer, L"TextVertexShader.cso");
	pixelShader = std::make_unique<PixelShader>(inRenderer, L"TextPixelShader.cso");

	std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	inputLayout = std::make_unique<InputLayout>(inRenderer, layout, vertexShader->GetByteCode());

	TextTransforms init{};
	D3D11_BUFFER_DESC cbd = {};
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.ByteWidth = sizeof(TextTransforms);
	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &init;
	inRenderer.GetDevice()->CreateBuffer(&cbd, &csd, &transformCBuffer);

	// linear: the atlas is already antialiased, this keeps scaled text smooth
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	inRenderer.GetDevice()->CreateSamplerState(&sd, &samplerState);

	screenVerts.reserve(2048);
	worldVerts.reserve(512);
}

TextRenderer::~TextRenderer() = default;

void TextRenderer::Begin()
{
	screenVerts.clear();
	worldVerts.clear();
	debugCursorY = 0.f;
}

void TextRenderer::SetScreenSize(const float width, const float height)
{
	screenWidth = width;
	screenHeight = height;
}

void TextRenderer::DrawScreen(std::string_view text, DirectX::XMFLOAT2 posPx,
	DirectX::XMFLOAT4 color, float scale, TextAlign align)
{
	// pixels (top-left origin, +y down) -> ortho world units (centre origin, +y up)
	const DirectX::XMFLOAT2 origin{
		posPx.x - screenWidth * 0.5f,
		screenHeight * 0.5f - posPx.y
	};

	Append(screenVerts, text, origin, color, scale, align);
}

void TextRenderer::DrawWorld(std::string_view text, DirectX::XMFLOAT2 worldPos,
	DirectX::XMFLOAT4 color, float scale, TextAlign align)
{
	Append(worldVerts, text, worldPos, color, scale, align);
}

void TextRenderer::DrawScreenF(DirectX::XMFLOAT2 posPx, DirectX::XMFLOAT4 color, const char* fmt, ...)
{
	char buffer[formatBufferSize];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, formatBufferSize, fmt, args);
	va_end(args);

	DrawScreen(buffer, posPx, color);
}

void TextRenderer::DrawWorldF(DirectX::XMFLOAT2 worldPos, DirectX::XMFLOAT4 color, const char* fmt, ...)
{
	char buffer[formatBufferSize];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, formatBufferSize, fmt, args);
	va_end(args);

	DrawWorld(buffer, worldPos, color);
}

void TextRenderer::DebugLine(const char* fmt, ...)
{
	char buffer[formatBufferSize];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, formatBufferSize, fmt, args);
	va_end(args);

	DrawScreen(buffer, { debugOrigin.x, debugOrigin.y + debugCursorY }, TextColor::White, debugScale);
	debugCursorY += font->GetLineHeight() * debugScale;
}

void TextRenderer::DebugLine(DirectX::XMFLOAT4 color, const char* fmt, ...)
{
	char buffer[formatBufferSize];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, formatBufferSize, fmt, args);
	va_end(args);

	DrawScreen(buffer, { debugOrigin.x, debugOrigin.y + debugCursorY }, color, debugScale);
	debugCursorY += font->GetLineHeight() * debugScale;
}

float TextRenderer::MeasureLine(std::string_view line, float scale)
{
	float width = 0.f;

	size_t i = 0;
	while (i < line.size())
	{
		const char32_t cp = NextCodepoint(line, i);

		if (cp == U'\t')
		{
			width += font->GetHalfAdvance() * 4.f * scale;
			continue;
		}

		const Glyph* glyph = font->GetGlyph(renderer, cp);
		width += (glyph ? glyph->advance : font->GetHalfAdvance()) * scale;
	}

	return width;
}

DirectX::XMFLOAT2 TextRenderer::Measure(std::string_view text, float scale)
{
	float widest = 0.f;
	size_t lines = 1;
	size_t lineBegin = 0;

	while (true)
	{
		const size_t newline = text.find('\n', lineBegin);
		const size_t lineEnd = (newline == std::string_view::npos) ? text.size() : newline;

		const float width = MeasureLine(text.substr(lineBegin, lineEnd - lineBegin), scale);
		widest = width > widest ? width : widest;

		if (newline == std::string_view::npos)
		{
			break;
		}

		lineBegin = newline + 1;
		++lines;
	}

	return { widest, static_cast<float>(lines) * font->GetLineHeight() * scale };
}

void TextRenderer::Append(std::vector<TextVertex>& out, std::string_view text, DirectX::XMFLOAT2 origin,
	DirectX::XMFLOAT4 color, float scale, TextAlign align)
{
	// everything is built in a +y up space; screen text converts its origin first
	const float lineStep = font->GetLineHeight() * scale;
	const float tabStep = font->GetHalfAdvance() * 4.f * scale;

	size_t lineBegin = 0;
	int lineIndex = 0;

	while (true)
	{
		const size_t newline = text.find('\n', lineBegin);
		const size_t lineEnd = (newline == std::string_view::npos) ? text.size() : newline;
		const std::string_view line = text.substr(lineBegin, lineEnd - lineBegin);

		float penX = origin.x;
		if (align != TextAlign::Left)
		{
			const float lineWidth = MeasureLine(line, scale);
			penX -= (align == TextAlign::Center) ? lineWidth * 0.5f : lineWidth;
		}

		const float top = origin.y - lineStep * static_cast<float>(lineIndex);

		size_t i = 0;
		while (i < line.size())
		{
			const char32_t cp = NextCodepoint(line, i);

			if (cp == U'\t')
			{
				penX += tabStep;
				continue;
			}

			const Glyph* glyph = font->GetGlyph(renderer, cp);
			if (!glyph)
			{
				penX += font->GetHalfAdvance() * scale; // atlas full: leave a blank slot
				continue;
			}

			if (glyph->width > 0.f) // spaces only move the pen
			{
				const float x0 = penX;
				const float x1 = penX + glyph->width * scale;
				const float y0 = top;                            // top edge
				const float y1 = top - glyph->height * scale;    // bottom edge

				const TextVertex tl{ {x0, y0, textDepth}, {glyph->uvMin.x, glyph->uvMin.y}, color };
				const TextVertex tr{ {x1, y0, textDepth}, {glyph->uvMax.x, glyph->uvMin.y}, color };
				const TextVertex br{ {x1, y1, textDepth}, {glyph->uvMax.x, glyph->uvMax.y}, color };
				const TextVertex bl{ {x0, y1, textDepth}, {glyph->uvMin.x, glyph->uvMax.y}, color };

				out.push_back(tl);
				out.push_back(tr);
				out.push_back(br);

				out.push_back(tl);
				out.push_back(br);
				out.push_back(bl);
			}

			penX += glyph->advance * scale;
		}

		if (newline == std::string_view::npos)
		{
			break;
		}

		lineBegin = newline + 1;
		++lineIndex;
	}
}

void TextRenderer::Flush(Renderer& inRenderer)
{
	if (screenVerts.empty() && worldVerts.empty())
	{
		return;
	}

	// ortho projection, depth off, alpha blending on
	inRenderer.Set2DMode();

	// world labels first, HUD on top
	FlushBatch(inRenderer, worldVerts, inRenderer.GetView());
	FlushBatch(inRenderer, screenVerts, DirectX::XMMatrixIdentity());
}

void TextRenderer::FlushBatch(Renderer& inRenderer, const std::vector<TextVertex>& verts, DirectX::XMMATRIX view)
{
	if (verts.empty())
	{
		return;
	}

	auto* ctx = inRenderer.GetContext().Get();

	EnsureCapacity(inRenderer, verts.size());
	{
		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, verts.data(), sizeof(TextVertex) * verts.size());
		ctx->Unmap(vertexBuffer.Get(), 0);
	}

	{
		TextTransforms t;
		t.model = DirectX::XMMatrixIdentity();
		t.view = DirectX::XMMatrixTranspose(view);
		t.projection = DirectX::XMMatrixTranspose(inRenderer.GetProj());

		D3D11_MAPPED_SUBRESOURCE msr;
		ctx->Map(transformCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
		memcpy(msr.pData, &t, sizeof(t));
		ctx->Unmap(transformCBuffer.Get(), 0);
	}

	// pipeline
	vertexShader->Bind(inRenderer);
	pixelShader->Bind(inRenderer);
	inputLayout->Bind(inRenderer);

	ctx->VSSetConstantBuffers(0, 1, transformCBuffer.GetAddressOf());

	ID3D11ShaderResourceView* atlasSrv = font->GetSrv();
	ctx->PSSetShaderResources(0, 1, &atlasSrv);
	ctx->PSSetSamplers(0, 1, samplerState.GetAddressOf());

	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const UINT stride = sizeof(TextVertex);
	const UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

	ctx->Draw(static_cast<UINT>(verts.size()), 0);
}

void TextRenderer::EnsureCapacity(Renderer& inRenderer, const size_t vertexCount)
{
	if (vertexCount <= vertexBufferCapacity && vertexBuffer)
	{
		return;
	}

	vertexBufferCapacity = vertexCount * 2 + 256;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = static_cast<UINT>(sizeof(TextVertex) * vertexBufferCapacity);
	bd.StructureByteStride = sizeof(TextVertex);

	vertexBuffer.Reset();
	inRenderer.GetDevice()->CreateBuffer(&bd, nullptr, &vertexBuffer);
}