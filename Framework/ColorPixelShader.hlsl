struct VSOut
{
    float4 color : COLOR;
    float4 pos : SV_POSITION;
};

float4 main(VSOut input) : SV_TARGET
{
    return input.color;
}