struct VertexInput {
    float3 pos     : POSITION;
    float3 normal  : NORMAL;
    float4 tangent : TANGENT;
    float2 uv      : TEXCOORD;
};

struct VertexOutput {
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 bitangent: BITANGENT;
    float2 uv       : TEXCOORD;
};

struct GBufferOutput {
    float4 albedo  : SV_Target0;
    float4 normals : SV_Target1;
};

struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
};

struct ModelPC {
    float4x4 model;
    float4x4 normalMat;
};

[[vk::binding(0, 1)]] ConstantBuffer<CameraUBO> camera : register(b0, space1);
[[vk::binding(1, 1)]] ConstantBuffer<ModelPC>   model  : register(b1, space1);

[[vk::binding(0, 2)]] Sampler2D albedoTex;
[[vk::binding(1, 2)]] Sampler2D normalTex;

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    float4 worldPos  = mul(model.model, float4(input.pos, 1.0));
    output.pos       = mul(camera.viewProj, worldPos);
    float3 N         = normalize(mul((float3x3)model.normalMat, input.normal));
    float3 T = normalize(mul((float3x3)model.model, input.tangent.xyz));
    output.normal = normalize(mul((float3x3)model.normalMat, input.normal));
    output.tangent = normalize(mul((float3x3)model.model, input.tangent.xyz));
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    output.uv        = input.uv;
    return output;
}

[shader("fragment")] 
GBufferOutput fragMain(VertexOutput input) {
    GBufferOutput output;

    output.albedo = albedoTex.Sample(input.uv);

    float3 ts = normalTex.Sample(input.uv).xyz * 2.0 - 1.0;
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);
    output.normals = float4(normalize(mul(TBN, ts)) * 0.5 + 0.5, 1.0);

    return output;
}