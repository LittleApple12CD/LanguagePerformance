struct VSInput {
    float3 pos    : TEXCOORD0;
    float3 axis   : TEXCOORD1;
    float3 center : TEXCOORD2;
    float  angle  : TEXCOORD3;
    float3 color  : TEXCOORD4;
};

struct VSOutput {
    float4 pos   : SV_Position;
    float3 color : TEXCOORD0;
    float3 localPos : TEXCOORD1;
};

cbuffer Uniforms : register(b0, space1) {
    float4x4 viewProj;
    float    scale;
    float3   pad;
};

float3 rotateAxis(float3 v, float3 axis, float a) {
    float c = cos(a), s = sin(a);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0 - c);
}

VSOutput main(VSInput input) {
    VSOutput o;
    o.localPos = input.pos;
    
    float3 local = input.pos * scale;
    float3 rotated = rotateAxis(local, normalize(input.axis), input.angle);
    float3 world = rotated + input.center;
    o.pos = mul(viewProj, float4(world, 1.0));
    o.color = input.color;
    return o;
}