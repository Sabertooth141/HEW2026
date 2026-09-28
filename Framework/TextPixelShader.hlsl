Texture2D fontAtlas : register(t0);
SamplerState fontSampler : register(s0);

struct VSOut
{
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float4 pos : SV_POSITION;
};

float4 main(VSOut input) : SV_TARGET
{
    // the atlas is white with the glyph coverage in alpha, so the vertex colour tints it
    float coverage = fontAtlas.Sample(fontSampler, input.uv).a;

    return float4(input.color.rgb, input.color.a * coverage);
}
