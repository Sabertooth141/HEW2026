#pragma once
#include <string>
#include <unordered_map>

#include "IComponent.h"
#include "DrawableBase.h"

struct MaterialData;
/**
 * handles materials, shaders, input layouts
 * binds vs, ps, inputLayout and matCBuffer
 */
class MaterialComponent : public IComponent
{
public:
	MaterialComponent(Renderer& renderer,
	                  const MaterialData& matData,
	                  const std::wstring& vsPath = L"VertexShader.cso",
	                  const std::wstring& psPath = L"PixelShader.cso");

	void Bind(Renderer& renderer);

private:
	static std::unordered_map<std::wstring, std::vector<std::unique_ptr<Bindable>>> staticBindsMap;
	std::vector<std::unique_ptr<Bindable>> instanceBinds;
	std::wstring shaderKey;
};
