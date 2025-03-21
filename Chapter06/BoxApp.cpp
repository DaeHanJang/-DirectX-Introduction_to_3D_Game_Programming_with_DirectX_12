//Direct3D 12에서 상자(정육면체)를 그리는 방법을 보여준다.
//조작 방법: 
// 왼쪽 버튼을 누른 채로 마우스를 움직이면 상자가 회전한다.
// 오른쪽 버튼을 누른 채로 마우스를 움직이면 상자가 커지거나 
// 작아진다.

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

struct Vertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
	//XMCOLOR Color;
};

struct VPosData {
	XMFLOAT3 Pos;
};

struct VColorData {
	XMFLOAT4 Color;
};

struct ObjectConstants {
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 PulseColor = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
	float Time = 0.f;
};

struct MyMeshGeometry : public MeshGeometry {
public:
	Microsoft::WRL::ComPtr<ID3DBlob> VertexPBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> VertexCBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexPBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> VertexCBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexPBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> VertexCBufferUploader = nullptr;

	UINT VertexPByteStride = 0;
	UINT VertexCByteStride = 0;
	UINT VertexPBufferByteSize = 0;
	UINT VertexCBufferByteSize = 0;

	D3D12_VERTEX_BUFFER_VIEW VertexPBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexPBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexPByteStride;
		vbv.SizeInBytes = VertexPBufferByteSize;

		return vbv;
	}

	D3D12_VERTEX_BUFFER_VIEW VertexCBufferView() const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexCBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = VertexCByteStride;
		vbv.SizeInBytes = VertexCBufferByteSize;

		return vbv;
	}
};

class BoxApp : public D3DApp {
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();
	
	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescripterHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayer();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	//std::unique_ptr<MyMeshGeometry> mBoxGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.f;

	POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	//디버그 빌드에서는 실행지점 메모리 점검 기능을 켠다.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		BoxApp theApp(hInstance);
		if (!theApp.Initialize()) return 0;

		return theApp.Run();
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

BoxApp::~BoxApp() {}

bool BoxApp::Initialize() {
	if (!D3DApp::Initialize()) return false;

	//초기화 명령들을 준비하기 위해 명령 목록을 재설정한다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescripterHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayer();
	BuildBoxGeometry();
	BuildPSO();

	//초기화 명령들을 실행한다.
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//초기화가 완료될 때까지 기다린다.
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize() {
	D3DApp::OnResize();
	//mScreenViewport.Width /= 2;
	//mScissorRect.bottom /= 2;
	//mScissorRect.right /= 2;

	//창의 크기가 바뀌었으므로 종횡비를 갱신하고 
	//투영 행렬을 다시 계산한다.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt) {
	//구면 좌표를 데카르트 좌표(직교 좌표)로 변환한다.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * sinf(mPhi) * sinf(mTheta);
	float z = mRadius * sinf(mPhi);

	//시야 행렬을 구축한다.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	//최신의 worldViewProj 행렬로 상수 버퍼를 갱신한다.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	objConstants.PulseColor = XMFLOAT4(Colors::Green);
	objConstants.Time = gt.TotalTime();
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt) {
	//명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 
	//재설정한다. 재설정은 GPU가 관련 명령 목록들을 
	//모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	//명령 목록을 ExecuteCommandList를 통해서 명령 대기열에 
	//추가했다면 명령 목록을 재설정할 수 있다. 명령 목록을 
	//재설정하면 메모리가 재활용된다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//후면 버퍼의 깊이 버퍼를 지운다.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), 
		Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.f, 0, 0, nullptr);

	//렌더링 결과가 기록될 렌더 대상 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	//mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexPBufferView());
	//mCommandList->IASetVertexBuffers(1, 1, &mBoxGeo->VertexCBufferView());
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	//mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
	//mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	//mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	mCommandList->SetGraphicsRootDescriptorTable(
		0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());

	mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);
	//mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["pyramid"].IndexCount, 1, 70, 196, 0);

	//자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//명령들의 기록은 마친다.
	ThrowIfFailed(mCommandList->Close());

	//명령 실행을 위해 명령 목록을 명령 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//후면 버퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//이 프레임의 명령들이 모두 처리되길 기다린다. 이러한 대기는 
	//비효율적이다. 이번에는 예제의 간단함을 위해 이 방법을 사용하지만, 
	//이후의 예제들에서는 렌더링 코드를 적절히 조직화해서 프레임마다 대기할 
	//필요가 없게 만든다.
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0) {
		//마우스 한 픽셀 이동을 4분의 1도에 대응시킨다.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		//마우스 입력에 기초해 각도를 갱신한다. 이에 의해 카메라가 상자를 중심으로 
		//공전하게 된다.
		mTheta += dx;
		mPhi += dy;

		//mPhi 각도를 제한한다.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if((btnState & MK_RBUTTON) != 0) {
		//마우스 한 픽셀 이동을 장면의 0.005단위에 대응시킨다.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		//마우스 입력에 기초해서 카메라 반지름을 갱신한다.
		mRadius += dx - dy;

		//반지름을 제한한다.
		mRadius = MathHelper::Clamp(mRadius, 3.f, 15.f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::BuildDescripterHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers() {
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();
	//버퍼에서 i번째 물체의 상수 버퍼의 오프셋을 얻는다.
	//지금은 i = 0이다.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature() {
	//일반적으로 셰이더 프로그램은 특정 자원들(상수 버퍼, 텍스처, 표본추출기 등)이 
	//입력된다고 기대한다. 루트 서명은 셰이더 프로그램이 기대하는 자원들을 
	//정의한다. 셰이더 프로그래은 본질적으로 하나의 함수이고 셰이더에 입력되는 
	//자원들은 함수의 매개변수들에 해당하므로, 루트 서명은 곧 함수 서명을 정의하는 
	//수단이라 한 수 있다.

	//루트 매개변수는 서술자 테이블이거나 루트 서술자 또는 루트 상수이다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	//CBV 하나를 담는 서술자 테이블을 생성한다.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	//루트 서명은 루트 매개변수들의 배열이다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//상수 버퍼 하나로 구성된 서술자 구간을 가리키는 
	//슬롯 하나로 이루어진 루트 서명을 생성한다.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayer() {
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout = { 
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		//{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		//{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void BoxApp::BuildBoxGeometry() {
	std::array<Vertex, 8> vertices = {
		Vertex({ XMFLOAT3(-1.f, -1.f, -1.f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.f, 1.f, -1.f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(1.f, 1.f, -1.f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(1.f, -1.f, -1.f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.f, -1.f, 1.f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.f, 1.f, 1.f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(1.f, 1.f, 1.f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(1.f, -1.f, 1.f), XMFLOAT4(Colors::Magenta) })

		//Vertex({ XMFLOAT3(-1.f, -1.f, 0.f), XMFLOAT4(Colors::Green) }),
		//Vertex({ XMFLOAT3(-1.f, 1.f, 0.f), XMFLOAT4(Colors::Green) }),
		//Vertex({ XMFLOAT3(1.f, 1.f, 0.f), XMFLOAT4(Colors::Green) }),
		//Vertex({ XMFLOAT3(1.f, -1.f, 0.f), XMFLOAT4(Colors::Green) }),
		//Vertex({ XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT4(Colors::Red) })

		//Vertex({ XMFLOAT3(-1.f, -1.f, -1.f), XMCOLOR(0.f, 0.f, 0.f, 1.f) }),
		//Vertex({ XMFLOAT3(-1.f, 1.f, -1.f), XMCOLOR(-1.f, 1.f, -1.f, 1.f) }),
		//Vertex({ XMFLOAT3(1.f, 1.f, -1.f), XMCOLOR(-1.f, 1.f, 1.f, 1.f) }),
		//Vertex({ XMFLOAT3(1.f, -1.f, -1.f), XMCOLOR(-1.f, -1.f, 1.f, 1.f) }),
		//Vertex({ XMFLOAT3(-1.f, -1.f, 1.f), XMCOLOR(1.f, -1.f, -1.f, 1.f) }),
		//Vertex({ XMFLOAT3(-1.f, 1.f, 1.f), XMCOLOR(1.f, 1.f, -1.f, 1.f) }),
		//Vertex({ XMFLOAT3(1.f, 1.f, 1.f), XMCOLOR(1.f, 1.f, 1.f, 1.f) }),
		//Vertex({ XMFLOAT3(1.f, -1.f, 1.f), XMCOLOR(1.f, -1.f, 1.f, 1.f) })
	};

	//std::array<VPosData, 13> verticesP = {
	//	VPosData({ XMFLOAT3(-1.f, -1.f, -1.f) }),
	//	VPosData({ XMFLOAT3(-1.f, 1.f, -1.f) }),
	//	VPosData({ XMFLOAT3(1.f, 1.f, -1.f) }),
	//	VPosData({ XMFLOAT3(1.f, -1.f, -1.f) }),
	//	VPosData({ XMFLOAT3(-1.f, -1.f, 1.f) }),
	//	VPosData({ XMFLOAT3(-1.f, 1.f, 1.f) }),
	//	VPosData({ XMFLOAT3(1.f, 1.f, 1.f) }),
	//	VPosData({ XMFLOAT3(1.f, -1.f, 1.f) }),
	//
	//	VPosData({ XMFLOAT3(-1.f, -1.f, 0.f) }),
	//	VPosData({ XMFLOAT3(-1.f, 1.f, 0.f) }),
	//	VPosData({ XMFLOAT3(1.f, 1.f, 0.f) }),
	//	VPosData({ XMFLOAT3(1.f, -1.f, 0.f) }),
	//	VPosData({ XMFLOAT3(0.f, 0.f, 1.f) })
	//};
	//
	//std::array<VColorData, 13> verticesC = {
	//	VColorData({ XMFLOAT4(Colors::White) }),
	//	VColorData({ XMFLOAT4(Colors::Black) }),
	//	VColorData({ XMFLOAT4(Colors::Red) }),
	//	VColorData({ XMFLOAT4(Colors::Green) }),
	//	VColorData({ XMFLOAT4(Colors::Blue) }),
	//	VColorData({ XMFLOAT4(Colors::Yellow) }),
	//	VColorData({ XMFLOAT4(Colors::Cyan) }),
	//	VColorData({ XMFLOAT4(Colors::Magenta) }), 
	//
	//	VColorData({ XMFLOAT4(Colors::Green) }),
	//	VColorData({ XMFLOAT4(Colors::Green) }),
	//	VColorData({ XMFLOAT4(Colors::Green) }),
	//	VColorData({ XMFLOAT4(Colors::Green) }),
	//	VColorData({ XMFLOAT4(Colors::Red) })
	//};

	std::array<std::uint16_t, 36> indices{
		//앞면
		0, 1, 2,
		0, 2, 3,
	
		//뒷면
		4, 6, 5,
		4, 7, 6,
	
		//왼쪽 면
		4, 5, 1,
		4, 1, 0,
	
		//오른쪽 면
		3, 2, 6,
		3, 6, 7,
	
		//윗면
		1, 5, 6,
		1, 6, 2,
	
		//아랫면
		4, 0, 3,
		4, 3, 7,

		//0, 1, 2,
		//0, 3, 2,
		//
		//0, 4, 1,
		//1, 4, 2,
		//2, 4, 3,
		//3, 4, 0
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	//const UINT vpbByteSize = (UINT)verticesP.size() * sizeof(VPosData);
	//const UINT vcbByteSize = (UINT)verticesC.size() * sizeof(VColorData);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	//mBoxGeo = std::make_unique<MyMeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	//ThrowIfFailed(D3DCreateBlob(vpbByteSize, &mBoxGeo->VertexPBufferCPU));
	//ThrowIfFailed(D3DCreateBlob(vcbByteSize, &mBoxGeo->VertexCBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	//CopyMemory(mBoxGeo->VertexPBufferCPU->GetBufferPointer(), verticesP.data(), vpbByteSize);
	//CopyMemory(mBoxGeo->VertexCBufferCPU->GetBufferPointer(), verticesC.data(), vcbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize,
		mBoxGeo->VertexBufferUploader);
	//mBoxGeo->VertexPBufferGPU = d3dUtil::CreateDefaultBuffer(
	//	md3dDevice.Get(), mCommandList.Get(),
	//	verticesP.data(), vpbByteSize,
	//	mBoxGeo->VertexBufferUploader);
	//mBoxGeo->VertexCBufferGPU = d3dUtil::CreateDefaultBuffer(
	//	md3dDevice.Get(), mCommandList.Get(),
	//	verticesC.data(), vcbByteSize,
	//	mBoxGeo->VertexBufferUploader);

	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize,
		mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	//mBoxGeo->VertexPByteStride = sizeof(VPosData);
	//mBoxGeo->VertexCByteStride = sizeof(VColorData);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	//mBoxGeo->VertexPBufferByteSize = vpbByteSize;
	//mBoxGeo->VertexCBufferByteSize = vcbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	//SubmeshGeometry submesh2;
	//submesh2.IndexCount = (UINT)indices.size();
	//submesh2.StartIndexLocation = (UINT)70;
	//submesh2.BaseVertexLocation = 196;

	mBoxGeo->DrawArgs["box"] = submesh;
	//mBoxGeo->DrawArgs["pyramid"] = submesh2;
}

void BoxApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), mvsByteCode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), mpsByteCode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//CD3DX12_RASTERIZER_DESC mRasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//mRasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//mRasterizer.CullMode = D3D12_CULL_MODE_FRONT;
	//psoDesc.RasterizerState = mRasterizer;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
