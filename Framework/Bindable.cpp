#include "Bindable.h"

ID3D11Device* Bindable::GetDevice(Renderer& renderer)
{
	return renderer.pDevice.Get();
}

ID3D11DeviceContext* Bindable::GetContext(Renderer& renderer)
{
	return renderer.pContext.Get();
}
