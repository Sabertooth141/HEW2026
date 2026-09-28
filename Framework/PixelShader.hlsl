cbuffer LightCBuf : register(b0)
{
	float3 lightPos;
	float padding0;
	float3 ambient;
	float padding1;
	float3 diffuseColor;
	float diffuseIntensity;
	float attConst;
	float attLin;
	float attQuad;
	float padding2;
}

cbuffer MatCBuf : register(b1)
{
	float3 matColor;
	float specularIntensity;
	float specularPower;
	float3 padding3;
}

Texture2D tex : register(t0);
SamplerState sampler0 : register(s0);

float4 main(
	float3 worldPos : POSITION,
	float3 normal : NORMAL,
	float2 texcoord : TEXCOORD
) : SV_TARGET
{
	normal = normalize(normal);

	float3 vToL = lightPos - worldPos;
	float dist = length(vToL);
	float3 dirToL = vToL / dist;
	float att = 1 / (attConst + attLin * dist + attQuad * (dist * dist));

	// diffuse
	float3 diffuse = diffuseColor * diffuseIntensity * att * max(0, dot(dirToL, normal));

	// specular
	float3 refl = reflect(-dirToL, normal);
	float3 toEye = normalize(-worldPos);
	float3 specular = att * diffuseColor * pow(max(0, dot(refl, toEye)), specularPower) * specularIntensity;

	// texture
    float4 texSample = tex.Sample(sampler0, texcoord);
    float3 texColor = any(texSample.rgb) ? texSample.rgb : float3(1, 1, 1);

	float3 result = saturate((diffuse + ambient) * texColor * matColor + specular);
	return float4(result, 1); 
}
