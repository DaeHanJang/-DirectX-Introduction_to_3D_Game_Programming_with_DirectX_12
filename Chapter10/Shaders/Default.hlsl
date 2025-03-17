//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

//���� �������� ���ǵǾ� ���� ������ �⺻������ �����Ѵ�.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

//������ ���� ����ü��� �Լ����� ���⿡ �����Ѵ�.
#include "LightingUtil.hlsl"

Texture2D    gDiffuseMap : register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

//�����Ӹ��� �޶����� ��� �ڷ�.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
};

//�������� �޶����� ��� �ڷ�.
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

	//���� ���α׷��� �Ȱ� �Ű��������� �����Ӻ��� ������ �� �ְ� �Ѵ�.
	//���� ��� �Ϸ� �� Ư�� �ð��뿡�� �Ȱ��� ������ ���� �ִ�.
	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 cbPerObjectPad2;

    //�ִ� MaxLights���� ��ü�� ���� �߿��� 
    //[0, NUM_DIR_LIGHTS) ������ ���ε��� ���Ɽ���̰� 
    //[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) ������ 
    //���ε��� �������̴�.
    //�׸��� [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, 
    //NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS) ������ 
    //���ε��� ���������̴�.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
	float4   gDiffuseAlbedo;
    float3   gFresnelR0;
    float    gRoughness;
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
	
    //���� �������� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    //���� ��Ŀ� ��յ� ��ʰ� ���ٰ� �����ϰ� ������ ��ȯ�Ѵ�.
    //��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    //���� ���� �������� ��ȯ�Ѵ�.
    vout.PosH = mul(posW, gViewProj);
	
	//��� ���� Ư������ ���� �ﰢ���� ���� �����ȴ�.
	float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
	vout.TexC = mul(texC, gMatTransform).xy;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST
	//�ؽ�ó ���İ� 0.1���� ������ �ȼ��� ����Ѵ�. ���̴� �ȿ��� 
	//�� ������ �ִ��� ���� �����ϴ� ���� �ٶ����ϴ�. �׷��� ��� �� 
	//���̴��� ������ �ڵ��� ������ ������ �� �����Ƿ� ȿ�����̴�.
	clip(diffuseAlbedo.a - 0.1f);
#endif

    //������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� �ٽ� ����ȭ�Ѵ�.
    pin.NormalW = normalize(pin.NormalW);

    //����Ǵ� ������ �������� ����.
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; //����ȭ

    //���� ��꿡 ���ԵǴ� �׵�.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

#ifdef FOG
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    //���� �ϴ� ��Ĵ��, �л� �������� ���ĸ� �����´�.
    litColor.a = diffuseAlbedo.a;

    return litColor;
}


