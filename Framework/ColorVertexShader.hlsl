cbuffer ModelBuffer : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
}

struct VSOut
{
	float4 color : COLOR;
	float4 pos : SV_POSITION;
};

VSOut main(float3 pos : POSITION, float4 color : COLOR)
{
	VSOut output;
	float4 position = float4(pos, 1);
	position = mul(position, world);
	position = mul(position, view);
	position = mul(position, projection);
	output.pos = position;
    output.color = color;

	return output;
}
