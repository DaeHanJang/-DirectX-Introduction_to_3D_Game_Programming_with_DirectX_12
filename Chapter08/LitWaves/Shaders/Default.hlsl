//***************************************************************************************
//Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
//�⺻ ���̴�. ������ ������.
//***************************************************************************************

//���� �������� ���ǵǾ� ���� ������ �⺻������ �����Ѵ�.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

//������ ���� ����ü��� �Լ����� ���⿡ �����Ѵ�.
#include "LightingUtil.hlsl"

//�����Ӹ��� �޶����� ��� �ڷ�
cbuffer cbPerObject : register(b0) {
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1) {
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
	float4x4 gMatTransform;
};

//���� ���� �޶����� ��� �ڷ�
cbuffer cbPass : register(b2) {
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

    //�ִ� MaxLights���� ��ü�� ���� �߿��� 
    //[0, NUM_DIR_LIGHTS) ������ ���ε��� ���Ɽ���̰� 
    //[NUM_DIR_LIGHTS, NUM_DIR_LIGHTS + NUM_POINT_LIGHTS) ������ 
    //���ε��� �������̴�.
    //�׸��� [NUM_DIR_LIGHTS + NUM_POINT_LIGHTS, NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS) ������ 
    //���ε��� ���������̴�.
    Light gLights[MaxLights];
};
 
//�����Ǵ� HLSL ���� ����ü
struct VertexIn {
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut {
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin) {
	VertexOut vout = (VertexOut)0.0f;
	
    //���� �������� ��ȯ�Ѵ�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    //���� ��Ŀ� ��յ� ��ʰ� ���ٰ� �����ϰ� ������ ��ȯ�Ѵ�.
    //��յ� ��ʰ� �ִٸ� ����ġ ����� ����ؾ� �Ѵ�.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    //���� ���� �������� ��ȯ�Ѵ�.
    vout.PosH = mul(posW, gViewProj);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    //������ �����ϸ� ���� ���̰� �ƴϰ� �� �� �����Ƿ� 
    //�ٽ� ����ȭ�Ѵ�.
    pin.NormalW = normalize(pin.NormalW);

    //����Ǵ� ������ �������� ����.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	//���� ������ �䳻 ���� �ֺ��� ��.
    float4 ambient = gAmbientLight*gDiffuseAlbedo;

    //���� ����.
    const float shininess = 1.0f - gRoughness;
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    //���� �ϴ� ��Ĵ��, �л� �������� ���ĸ� �����´�.
    litColor.a = gDiffuseAlbedo.a;

    return litColor;
}


