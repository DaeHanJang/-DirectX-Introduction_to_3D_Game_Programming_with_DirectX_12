//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "MathHelper.h"

extern const int gNumFrameResources;

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}

inline std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description) 
    #endif
#endif 		
    */

class d3dUtil
{
public:

    static bool IsKeyDown(int vkeyCode);

    static std::string ToString(HRESULT hr);

    static UINT CalcConstantBufferByteSize(UINT byteSize) {
        //상수 버퍼의 크기는 반드시 최소 하드웨어 할당 크기(흔히 256바이트)의 
        //배수이어야 한다. 이 메서드는 주어진 크기에 가장 가까운 256의 
        //배수를 구해서 돌려준다. 이를 위해 이 메서드는 크기에 255를 더하고 
        //비트마스크를 이용해서 하위 2바이트, 즉 256보다 작은 모든 비트를 0으로 
        //만든다. 예: byteSize = 300이라 할 때.
        //(300 + 255) & ~255
        //555 & ~255
        //0x022B & ~0x00ff
        //0x022B & 0xff00
        //0x0200
        //512
        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

class DxException {
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};

//이 구조체는 MeshGeometry가 대표하는 기하구조 그룹(메시)의 부분 구간, 부분 
//메시를 정의한다. 부분 메시는 하나의 정점/색인 버퍼에 여러 개의 기하구조가 
//들어 있는 경우에 쓰인다. 이 구조체는 정점/색인 버퍼에 저장된 메시의 부분 
//메시를 그리는 데 필요한 오프셋들과 자료를 제공한다.
struct SubmeshGeometry {
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

    //이 부분 메시가 정의하는 기하구조의 경계 상자(bouding box).
	DirectX::BoundingBox Bounds;
};

struct MeshGeometry {
	//이 메시를 이름으로 조회할 수 있도록 이름을 부여한다.
	std::string Name;

	//시스템 메모리 복사본. 정점/색인 형식이 범용적일 수 있으므로 
	//블로브(ID3DBlob)를 사용한다.
    //실제로 사용할 때에는 클라이언트에서 적절한 캐스팅해야 한다.
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU  = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

    //버퍼들에 관한 자료
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	//한 MeshGeometry 인스턴스의 한 정점/색인 버퍼에 여러 개의 
	//기하구조를 담을 수 있다.
	//부분 메시들을 개별적으로 그릴 수 있도록, 부분 메시 기하구조들을 
    //컨테이너에 담아 둔다.
	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexByteStride;
		vbv.SizeInBytes = VertexBufferByteSize;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	//자료를 GPU에 모두 올린 후에는 메모리를 해제해도 된다.
	void DisposeUploaders() {
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
};

struct Light {
    DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };  //빛의 색상.
    float FalloffStart = 1.0f;                          //점광과 점적광에만 쓰인다.
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };//지향광과 점적광에만 쓰인다.
    float FalloffEnd = 10.0f;                           //점광과 점적광에만 쓰인다.
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  //점광과 점적광에만 쓰인다.
    float SpotPower = 64.0f;                            //지향광과 점적광에만 쓰인다.
};

#define MaxLights 16

struct MaterialConstants {
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.f, 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

//예제들에서 재질을 나타내는 데 쓰이는 간단한 구조체
struct Material {
	//고유한 재질 이름(재질 조회에 쓰임).
	std::string Name;

	//이 재질에 해당하는 상수 버퍼의 색인
	int MatCBIndex = -1;

	//SRV 힙의 분산 반사율 텍스처 색인
	int DiffuseSrvHeapIndex = -1;

	// Index into SRV heap for normal texture.
	int NormalSrvHeapIndex = -1;

	//재질이 변해서 해당 상수 버퍼를 갱신해야 하는지의 여부를 
	//나타내는 더러움 플래그. FrameResource마다 물체의 cbuffer가 
	//있으므로, FrameResource마다 갱신을 적용해야 한다. 따라서, 
	//물체의 자료를 수정할 때에는 반드시 
    //NumFramesDirty = gNumFrameResources
    //로 설정해야 한다. 그래야 각각의 프레임 자원이 갱신된다.
	int NumFramesDirty = gNumFrameResources;

	//셰이딩에 쓰이는 재질 상수 버퍼 재료.
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.f, 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

//텍스처 관련 자료를 한데 묶어 편하게 관리하기 위한 구조체
struct Texture {
	//조회 시 사용할 고유한 재질 이름.
	std::string Name;

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif