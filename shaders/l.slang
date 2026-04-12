struct VertexOutput {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

struct LightUBO {
    float3 direction;
    float intensity;
    float3 color;
    float _pad;
};

[[vk::binding(0, 3)]] ConstantBuffer<LightUBO> light : register(b0, space3);

[[vk::binding(0, 2)]] Sampler2D albedoTex;
[[vk::binding(1, 2)]] Sampler2D normalTex;

[shader("vertex")]
VertexOutput vertMain(uint id : SV_VertexID) {
    VertexOutput output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.pos = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.uv.y = 1.0 - output.uv.y;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target0 {
    float3 albedo = albedoTex.Sample(input.uv).rgb;
    float3 normal = normalTex.Sample(input.uv).xyz * 2.0 - 1.0;

    float NdotL = max(dot(normal, normalize(-light.direction)), 0.0);
    float3 ambient = albedo * 0.1;
    float3 diffuse = albedo * light.color * light.intensity * NdotL;

    return float4(ambient + diffuse, 1.0);
}