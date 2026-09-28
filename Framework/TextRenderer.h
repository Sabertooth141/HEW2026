#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <string_view>
#include <vector>
#include <wrl/client.h>

#include "BitmapFont.h"

class InputLayout;
class PixelShader;
class VertexShader;
class Renderer;

struct TextVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT4 color;
};

enum class TextAlign
{
	Left,
	Center,
	Right
};

namespace TextColor
{
	inline constexpr DirectX::XMFLOAT4 White{ 1.f, 1.f, 1.f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Black{ 0.f, 0.f, 0.f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Red{ 1.f, 0.25f, 0.25f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Green{ 0.35f, 1.f, 0.35f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Blue{ 0.4f, 0.6f, 1.f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Yellow{ 1.f, 0.9f, 0.3f, 1.f };
	inline constexpr DirectX::XMFLOAT4 Cyan{ 0.4f, 1.f, 1.f, 1.f };
}

// Immediate-mode text. Same shape as DebugRenderer: queue during the frame,
// one Flush at the end that uploads everything and issues one draw per space.
//
//   Screen space - pixels, (0,0) is the top-left corner of the window, +y down.
//                  Ignores the camera. This is the HUD / debug overlay.
//   World space  - same units as sprites and physics, +y up, moves with Camera2D.
//                  This is for labels pinned to objects.
//
// All text is UTF-8. Build with /utf-8 so "残り3秒" in a source file is UTF-8
// rather than the system codepage.
class TextRenderer
{
public:
	explicit TextRenderer(Renderer& renderer, const wchar_t* fontName = L"MS Gothic", int fontPixelHeight = 18);
	~TextRenderer();

	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;

	// clears the queue and resets the debug line cursor
	void Begin();
	void Flush(Renderer& renderer);

	void DrawScreen(std::string_view text, DirectX::XMFLOAT2 posPx,
		DirectX::XMFLOAT4 color = TextColor::White,
		float scale = 1.f, TextAlign align = TextAlign::Left);

	void DrawWorld(std::string_view text, DirectX::XMFLOAT2 worldPos,
		DirectX::XMFLOAT4 color = TextColor::White,
		float scale = 1.f, TextAlign align = TextAlign::Center);

	// printf-style variants. %s takes UTF-8 bytes and passes through unharmed.
	void DrawScreenF(DirectX::XMFLOAT2 posPx, DirectX::XMFLOAT4 color, const char* fmt, ...);
	void DrawWorldF(DirectX::XMFLOAT2 worldPos, DirectX::XMFLOAT4 color, const char* fmt, ...);

	// stacked lines down the corner of the screen, one call per line
	void DebugLine(const char* fmt, ...);
	void DebugLine(DirectX::XMFLOAT4 color, const char* fmt, ...);

	void SetDebugOrigin(DirectX::XMFLOAT2 posPx) { debugOrigin = posPx; }
	void SetDebugScale(float scale) { debugScale = scale; }

	// size the text would occupy, handy for boxes/anchoring.
	// not const: an unseen glyph gets rasterized into the atlas on the way.
	DirectX::XMFLOAT2 Measure(std::string_view text, float scale = 1.f);

	void SetScreenSize(float width, float height);

	BitmapFont& GetFont() { return *font; }

private:
	void Append(std::vector<TextVertex>& out, std::string_view text, DirectX::XMFLOAT2 origin,
		DirectX::XMFLOAT4 color, float scale, TextAlign align);

	float MeasureLine(std::string_view line, float scale);

	void EnsureCapacity(Renderer& inRenderer, size_t vertexCount);
	void FlushBatch(Renderer& renderer, const std::vector<TextVertex>& verts, DirectX::XMMATRIX view);

private:
	Renderer& renderer; // glyphs are cached lazily, so drawing needs the device

	std::vector<TextVertex> screenVerts;
	std::vector<TextVertex> worldVerts;

	std::unique_ptr<BitmapFont> font;
	std::unique_ptr<VertexShader> vertexShader;
	std::unique_ptr<PixelShader> pixelShader;
	std::unique_ptr<InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> transformCBuffer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	size_t vertexBufferCapacity = 0;

	float screenWidth = 0.f;
	float screenHeight = 0.f;

	DirectX::XMFLOAT2 debugOrigin{ 8.f, 8.f };
	float debugScale = 1.f;
	float debugCursorY = 0.f;

	static constexpr float textDepth = 0.5f; // depth test is off in 2D mode, draw order decides
};