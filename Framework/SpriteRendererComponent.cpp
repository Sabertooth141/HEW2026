#include "SpriteRendererComponent.h"

#include "MaterialComponent.h"
#include "GameObject.h"

SpriteRendererComponent::SpriteRendererComponent(Renderer& renderer, ID3D11ShaderResourceView* srv)
    : renderer(renderer)
    , uvCBuffer(renderer, UVTransformData{ {0.f, 0.f}, {1.f, 1.f} }, 2) // b2
    , texture(srv, 0)    // t0
    , sampler(renderer, 0) // s0

{
}

void SpriteRendererComponent::Update(float deltaTime)
{
	IComponent::Update(deltaTime);
}

void SpriteRendererComponent::Bind(Renderer& renderer)
{
    uvCBuffer.Bind(renderer);
    texture.Bind(renderer);
    sampler.Bind(renderer);
}

void SpriteRendererComponent::UpdateUV(const UVTransformData& data)
{
    uvCBuffer.Update(renderer, data);
}

void SpriteRendererComponent::Render() const
{
    MaterialComponent* comp = owner->GetComponent<MaterialComponent>();
    comp->Bind(renderer);
    renderer.SetSpriteFlip(flipX, flipY);
    for (auto& mesh : owner->GetMeshes())
    {
        mesh->Draw(renderer);
    }
}
