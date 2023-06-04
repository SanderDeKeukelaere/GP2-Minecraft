//=============================================================================
//// Shader uses position and texture
//=============================================================================
SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Mirror;
    AddressV = Mirror;
};

float gNonRedDarkener = 0.5f;
float gOverallDarkener = 1.0f;
Texture2D gTexture;

float gVignetteStrength = 4.0f;

// Create Depth Stencil State (ENABLE DEPTH WRITING)
DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
};

/// Create Rasterizer State (Backface culling) 
RasterizerState BackCulling
{
    CullMode = BACK;
};

//IN/OUT STRUCTS
//--------------
struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;

};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD1;
};


//VERTEX SHADER
//-------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Position = float4(input.Position, 1.0f);
    output.TexCoord = input.TexCoord;
	
    return output;
}


//PIXEL SHADER
//------------
float4 PS(PS_INPUT input) : SV_Target
{
    float2 dist = input.TexCoord - 0.5f;
    dist.x = 1 - dot(dist, dist);
    float vignette = saturate(pow(dist.x, gVignetteStrength));
    
    float4 color = gTexture.Sample(samPoint, input.TexCoord);
    
    float3 albedo = float3(color.r * vignette, color.gb * vignette * gNonRedDarkener);
    
    return float4(albedo * gOverallDarkener, 1.0f);
}


//TECHNIQUE
//---------
technique11 Grayscale
{
    pass P0
    {
        SetRasterizerState(BackCulling);
        SetDepthStencilState(EnableDepth, 0);
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
