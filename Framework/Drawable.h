#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>

class Bindable;
class IndexBuffer;
class Renderer;

class Drawable
{
	template<class T>
	friend class DrawableBase;

public:

	Drawable() = default;
	virtual ~Drawable() = default;
	Drawable(const Drawable&) = delete;

	//virtual void Update(float deltaTime) = 0;
	//virtual DirectX::XMMATRIX GetTransformMatrix() = 0;
	virtual void Draw(Renderer& renderer);

protected:
	void AddBind(std::unique_ptr<Bindable> bind);
	void AddIndexBuffer(std::unique_ptr<IndexBuffer> iBuffer);

private:
	virtual const std::vector<std::unique_ptr<Bindable>>& GetStaticBinds() = 0;

protected:
	IndexBuffer* pIndexBuffer = nullptr;
	std::vector<std::unique_ptr<Bindable>> bindings;
};

