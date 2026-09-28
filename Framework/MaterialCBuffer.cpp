#include "MaterialCBuffer.h"

MaterialCBuffer::MaterialCBuffer(Renderer& renderer, const MaterialData& data) : pBuffer(renderer, data, 1)
{
}

void MaterialCBuffer::Bind(Renderer& renderer)
{
	pBuffer.Bind(renderer);
}
