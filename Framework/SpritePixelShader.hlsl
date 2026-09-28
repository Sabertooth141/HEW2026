Texture2D tex : register(t0);
SamplerState sam : register(s0);

cbuffer SpriteBuffer : register(b2)
{
    float2 uvOffset;
    float2 uvScale;
};

cbuffer SpriteData : register(b3)
{
    int flipX;
    int flipY;
    float2 padding;
}

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    float2 localUV = uv;
    
	if (flipX)
	{
        localUV.x = 1.0f - localUV.x;
    }
	if (flipY)
	{
        localUV.y = 1.0f - localUV.y;
    }

    float2 finalUV = uvOffset + localUV * uvScale;

    float4 result = tex.Sample(sam, finalUV);
    clip(result.a - 0.1f);
    return result;
} 