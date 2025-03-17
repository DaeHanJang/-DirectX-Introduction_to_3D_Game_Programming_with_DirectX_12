//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

//광원 개수들이 정의되어 있지 않으면 기본값으로 정의한다.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

//조명을 위한 구조체들과 함수들을 여기에 포함한다.
#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamLinear  : register(s0);
//SamplerState gsamPointWrap : register(s0);
//SamplerState gsamPointClamp : register(s1);
//SamplerState gsamLinearWrap : register(s2);
//SamplerState gsamLinearClamp : register(s3);
//SamplerState gsamAnisotropicWrap : register(s4);
//SamplerState gsamAnisotropicClamp : register(s5);

//프레임마다 달라지는 상수 자료.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
};

//재질마다 달라지는 상수 자료.
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    //최대 MaxLights개의 물체별 광원 중에서 
    //[0, NUM_DIR_LIGHTS) 구간의 색인들은 지향광들이고 
    //[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) 구간의 
    //색인들은 점광들이다.
    //그리고 [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, 
    //NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS) 구간의
    //색인들은 점광들이다.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
	float2 TexC    : TEXCOORD;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    //세계 공간으로 변환한다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    //세계 행렬에 비균등 비례가 없다고 가정하고 법선을 변환한다.
    //비균등 비례가 있다면 역전치 행렬을 사용해야 한다.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    //동차 절단 공간으로 변환한다.
    vout.PosH = mul(posW, gViewProj);
	
	//텍스처 좌표를 출력 정점의 해당 특성에 설정한다.
    //출력 정점 특성들은 이후 삼각형을 따라 보간된다.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //이 픽셀에서의 분산광 반사율을 텍스처에서 가져온다.
    //텍스처에서 추출한 반사율에 상수 버퍼에 있는 반사율을 곱한다.
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinear, pin.TexC) * gDiffuseAlbedo;

    //법선을 보간하면 단위 길이가 아니게 될 수 있으므로 
    //다시 정규화한다.
    pin.NormalW = normalize(pin.NormalW);

    //조명되는 점에서 눈으로의 벡터.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    //조명 계산에 포함되는 항들.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    //흔히 하는 방식대로, 분산 재질에서 알파를 가져온다.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}


