#include "LightCBuffer.h"

LightCBuffer::LightCBuffer(Renderer& renderer, const LightData& data) : data(data), pBuffer(renderer, data, 0)
{
}

void LightCBuffer::Update(Renderer& renderer, const LightData& newData)
{
	data = newData;
	pBuffer.Update(renderer, data);
}

void LightCBuffer::Bind(Renderer& renderer)
{
	pBuffer.Bind(renderer);
}
