#pragma once
#include "IComponent.h"
#include "DrawableBase.h"

class TransformComponent : public IComponent
{
public:
	TransformComponent(DirectX::XMFLOAT3 pos = {0, 0, 0},
	                   DirectX::XMFLOAT3 rot = {0, 0, 0},
	                   DirectX::XMFLOAT3 scale = {1, 1, 1});

	void SetPosition(DirectX::XMFLOAT3 inPosition);
	void SetRotation(DirectX::XMFLOAT3 inRotation);

	/**
	 * forcibly set current scale, ignoring former aspect ratio
	 * 
	 * @param inScale scale to set
	 */
	void SetScale(DirectX::XMFLOAT3 inScale);

	/**
	 * used to change current scale with sprites' current aspect ratio 
	 * 
	 * @param inScale scale to scale up or down
	 */
	void SetScale(float inScale);

	void Rotate(DirectX::XMFLOAT3 inRotate);
	void Translate(DirectX::XMFLOAT3 inTranslate);

	DirectX::XMFLOAT3 GetPosition() const { return position; }
	DirectX::XMFLOAT3 GetRotation() const { return rotation; }
	DirectX::XMFLOAT3 GetScale() const { return scale; }

	DirectX::XMMATRIX GetMatrix() const;

	void Update(float deltaTime) override;

private:
	DirectX::XMFLOAT3 position = {0, 0, 0};
	DirectX::XMFLOAT3 rotation = {0, 0, 0};
	DirectX::XMFLOAT3 scale = {0, 0, 0};
};
