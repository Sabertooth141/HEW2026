#include "MaterialComponent.h"

#include "InputLayout.h"
#include "MaterialCBuffer.h"
#include "PixelShader.h"
#include "VertexShader.h"

std::unordered_map<std::wstring, std::vector<std::unique_ptr<Bindable>>> MaterialComponent::staticBindsMap;

MaterialComponent::MaterialComponent(Renderer& renderer,
                                     const MaterialData& matData,
                                     const std::wstring& vsPath,
                                     const std::wstring& psPath)
{
	shaderKey = vsPath + L"|" + psPath;
	if (!staticBindsMap.contains(shaderKey))
	{
		std::vector<std::unique_ptr<Bindable>>& binds = staticBindsMap[shaderKey];
		auto vShader = std::make_unique<VertexShader>(renderer, vsPath);
		auto vsByteCode = vShader->GetByteCode();

		binds.push_back(std::move(vShader));
		binds.push_back(std::make_unique<PixelShader>(renderer, psPath));

		const std::vector<D3D11_INPUT_ELEMENT_DESC> layout =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

		binds.push_back(std::make_unique<InputLayout>(renderer, layout, vsByteCode));
	}

	instanceBinds.push_back(std::make_unique<MaterialCBuffer>(renderer, matData));
}

void MaterialComponent::Bind(Renderer& renderer)
{
	for (auto& staticBind : staticBindsMap[shaderKey])
	{
		staticBind->Bind(renderer);
	}

	for (auto& bind : instanceBinds)
	{
		bind->Bind(renderer);
	}
}
