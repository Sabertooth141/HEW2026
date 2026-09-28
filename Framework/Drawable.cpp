#include "Drawable.h"

#include "Bindable.h"
#include "IndexBuffer.h"

void Drawable::Draw(Renderer& renderer)
{
	for (auto& iBind: GetStaticBinds())
	{
		iBind->Bind(renderer);
	}

	for (auto& iBind : bindings)
	{
		iBind->Bind(renderer);
	}

	renderer.DrawIndexed(pIndexBuffer->GetIndexCount());
}

void Drawable::AddBind(std::unique_ptr<Bindable> bind)
{
	assert("*MUST* use AddIndexBuffer to bind index buffer" && typeid(*bind) != typeid(IndexBuffer));
	bindings.push_back(std::move(bind));
}

void Drawable::AddIndexBuffer(std::unique_ptr<IndexBuffer> iBuffer)
{
	assert("Attempting to add index buffer a second time" && pIndexBuffer == nullptr);
	pIndexBuffer = iBuffer.get();
	bindings.push_back(std::move(iBuffer));
}
