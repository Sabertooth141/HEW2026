cbuffer ModelBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
}

struct VSOut
{
    float2 uv : TEXCOORD;
    float4 pos : SV_POSITION;
};

VSOut main(float3 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
    VSOut output;
    float4 posOut = float4(pos, 1);
	posOut = mul(posOut, world);
    posOut = mul(posOut, view);
    posOut = mul(posOut, projection);
    output.pos = posOut;
    output.uv = uv;

    return output;
}