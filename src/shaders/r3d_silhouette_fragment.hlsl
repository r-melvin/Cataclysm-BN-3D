Texture2D<float4> atlas : register(t0, space2);
SamplerState atlas_sampler : register(s0, space2);
struct FragmentInput {
    float2 uv : TEXCOORD0;
};
float4 main(FragmentInput input) : SV_Target0 {
    const float alpha = atlas.Sample(atlas_sampler, input.uv).a;
    if (alpha < 0.75) { discard; }
    return float4(0.15, 0.85, 1.0, 0.62);
}
