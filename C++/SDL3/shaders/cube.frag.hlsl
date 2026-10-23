struct PSInput {
    float4 pos   : SV_Position;
    float3 color : TEXCOORD0;
    float3 localPos : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target {
    float3 absPos = abs(input.localPos);

    float maxAxis = max(max(absPos.x, absPos.y), absPos.z);

    float isEdge = 0.0;
    if (maxAxis > 0.49) {
        if (absPos.x == maxAxis && (absPos.y > 0.48 || absPos.z > 0.48)) isEdge = 1.0;
        else if (absPos.y == maxAxis && (absPos.x > 0.48 || absPos.z > 0.48)) isEdge = 1.0;
        else if (absPos.z == maxAxis && (absPos.x > 0.48 || absPos.y > 0.48)) isEdge = 1.0;
    }

    if (isEdge > 0.5) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    return float4(input.color, 1.0);
}