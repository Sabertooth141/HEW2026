#pragma once
#include "Drawable.h"
#include "IndexBuffer.h"

template <class T>
class DrawableBase : public Drawable
{
protected:
	static bool IsStaticInit()
	{
		return !staticBinds.empty();
	}

	static void AddStaticBind(std::unique_ptr<Bindable> bind)
	{
		assert("*must* use AddStaticIndexBuffer for index buffers" && typeid(*bind) != typeid(IndexBuffer));
		staticBinds.push_back(std::move(bind));
	}

	void AddStaticIndexBuffer(std::unique_ptr<IndexBuffer> iBuffer)
	{
		assert("Attempting to add index buffer a second time" && pIndexBuffer == nullptr);
		pIndexBuffer = iBuffer.get();
		staticBinds.push_back(std::move(iBuffer));
	}

	void SetIndexFromStatic()
	{
		for (const auto& bind : staticBinds)
		{
			if (const auto p = dynamic_cast<IndexBuffer*>(bind.get()))
			{
				pIndexBuffer = p;
				return;
			}
		}
		assert("failed to find index buffer in static binds" && false);
	}
private:
	std::vector<std::unique_ptr<Bindable>>& GetStaticBinds() override
	{
		return staticBinds;
	}

private:
	static std::vector<std::unique_ptr<Bindable>> staticBinds;
};

template<class T>
std::vector<std::unique_ptr<Bindable>> DrawableBase<T>::staticBinds;
