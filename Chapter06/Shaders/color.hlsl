//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

//static const float PI = 3.14159265f;

cbuffer cbPerObject : register(b0) {
	float4x4 gWorldViewProj; 
    float4 gPulseColor;
    float gTime;
};

struct VertexIn {
	float3 PosL  : POSITION;
    float4 Color : COLOR;
};

struct VertexOut {
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
    //float gTime : TIME;
};

VertexOut VS(VertexIn vin) {
	VertexOut vout;
	
    //vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime.x);
    //vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime.x);
	
    //vin.Color += -(cos(PI * gTime.x) - 1) / 2;
    
	//���� ���� �������� ��ȯ�Ѵ�.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    
	//���� ������ �״�� �ȼ� ���̴��� �����Ѵ�.
    vout.Color = vin.Color;
    
    //vout.gTime = gTime.x;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    //pin.Color += -(cos(PI * pin.gTime) - 1) / 2;
    
    //clip(pin.Color.r - 0.5f);
    
    //return pin.Color;
    
    const float pi = 3.14159;
    
    //���� �Լ��� �̿��ؼ�, �ð��� ���� [0, 1] �������� �����ϴ� ���� ���Ѵ�.
    float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;
    
    //�Ű����� s�� �����ؼ� pin.Color�� gPulseColor ���̸� �Ų����� 
    //������ ���� ���Ѵ�.
    float4 c = lerp(pin.Color, gPulseColor, s);
    
    return c;
}
