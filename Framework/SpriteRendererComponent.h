#pragma once
#include <DirectXMath.h>
#include <string>

#include "ConstantBuffer.h"
#include "Drawable.h"
#include "IComponent.h"
#include "PixelShader.h"
#include "RenderLayer.h"
#include "Sampler.h"
#include "Texture.h"
#include "UVTransformData.h"

class SpriteRendererComponent : public IComponent
{
public:
	SpriteRendererComponent(Renderer& renderer, ID3D11ShaderResourceView* srv);

	void Update(float deltaTime) override;
	void Bind(Renderer& renderer);
	void UpdateUV(const UVTransformData& data);
	void Render() const;

	Texture& GetTexture() { return texture; }

public:
	RenderLayer layer = RenderLayer::Default;
	int sortOrder = 0;
	bool enabled = true;

	bool flipX = false;
	bool flipY = false;

private:
	Renderer& renderer;
	PixelConstantBuffer<UVTransformData> uvCBuffer;
	Texture texture;
	Sampler sampler;
};

