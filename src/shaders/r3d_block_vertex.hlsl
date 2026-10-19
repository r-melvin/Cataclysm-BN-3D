// Port of the CDDA fork's r3d_block.vert; SDL_GPU uses column-major matrices.
cbuffer params : register(b0, space1) {
    column_major float4x4 view_proj;
    float4 offset;
    float4 pull;
    float4 cut;
    float4 cut2;
    float4 fog;
    float4 fog2;
};
struct VertexInput {
    float3 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 light : TEXCOORD2;
};
struct VertexOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 light : TEXCOORD1;
    noperspective float3 ndc : TEXCOORD2;
    float height : TEXCOORD3;
    nointerpolation float4 cut : TEXCOORD4;
    nointerpolation float2 cut2 : TEXCOORD5;
    float fog_amount : TEXCOORD6;
    nointerpolation float4 fog_color : TEXCOORD7;
};
VertexOutput main(VertexInput input) {
    VertexOutput output;
    const float3 position = input.position + offset.xyz;
    float4 projected = mul(view_proj, float4(position, 1.0));
    if (pull.w > 0.0) {
        const float4 pulled = mul(view_proj, float4(position + pull.xyz * (pull.w * input.light.a), 1.0));
        projected.z = pulled.z / pulled.w * projected.w;
    }
    output.position = projected;
    output.uv = input.uv;
    output.light = float4(input.light.rgb, 1.0);
    output.ndc = projected.xyz / projected.w;
    output.height = position.z;
    output.cut = cut;
    output.cut2 = cut2.xy;
    const float distance = length(position.xy - fog2.xy) * fog.w;
    output.fog_amount = 1.0 - exp(-distance * distance);
    output.fog_color = float4(fog.rgb, fog2.z);
    return output;
}
