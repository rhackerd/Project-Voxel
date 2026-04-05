struct VertexInput {
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD;
};

struct VertexOutput {
    float4 pos      : SV_Position;
    float3 normal   : NORMAL;
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

ConstantBuffer<CameraUBO> camera : register(b0, space1);

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.pos    = mul(camera.viewProj, float4(input.pos, 1.0));
    output.normal = input.normal;
    output.uv     = input.uv;
    return output;
}

[shader("fragment")]
GBufferOutput fragMain(VertexOutput input) {
    GBufferOutput output;
    output.albedo  = float4(1.0, 1.0, 1.0, 1.0); // white until textures
    output.normals = float4(input.normal * 0.5 + 0.5, 1.0); // encode to [0,1]
    return output;
}