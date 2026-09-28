cbuffer Transform : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct VSOutput
{
	float3 worldPos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float4 pos : SV_POSITION;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	float4 worldPos = mul(float4(input.pos, 1), model);
	output.worldPos = worldPos.xyz;

	output.normal = normalize(mul(input.normal, (float3x3)model));

	output.texcoord = input.texcoord;

	float4 clipPos = mul(worldPos, view);
	clipPos = mul(clipPos, projection);
	output.pos = clipPos;


	return output;
} 
