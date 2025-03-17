//Direct3D 12���� ����(������ü)�� �׸��� ����� �����ش�.
//���� ���: 
// ���� ��ư�� ���� ä�� ���콺�� �����̸� ���ڰ� ȸ���Ѵ�.
// ������ ��ư�� ���� ä�� ���콺�� �����̸� ���ڰ� Ŀ���ų� 
// �۾�����.

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
	//����� ���忡���� �������� �޸� ���� ����� �Ҵ�.
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

	//�ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ����� �缳���Ѵ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescripterHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayer();
	BuildBoxGeometry();
	BuildPSO();

	//�ʱ�ȭ ��ɵ��� �����Ѵ�.
	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ʱ�ȭ�� �Ϸ�� ������ ��ٸ���.
	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize() {
	D3DApp::OnResize();
	//mScreenViewport.Width /= 2;
	//mScissorRect.bottom /= 2;
	//mScissorRect.right /= 2;

	//â�� ũ�Ⱑ �ٲ�����Ƿ� ��Ⱦ�� �����ϰ� 
	//���� ����� �ٽ� ����Ѵ�.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt) {
	//���� ��ǥ�� ��ī��Ʈ ��ǥ(���� ��ǥ)�� ��ȯ�Ѵ�.
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float y = mRadius * sinf(mPhi) * sinf(mTheta);
	float z = mRadius * sinf(mPhi);

	//�þ� ����� �����Ѵ�.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	//�ֽ��� worldViewProj ��ķ� ��� ���۸� �����Ѵ�.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	objConstants.PulseColor = XMFLOAT4(Colors::Green);
	objConstants.Time = gt.TotalTime();
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt) {
	//��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� 
	//�缳���Ѵ�. �缳���� GPU�� ���� ��� ��ϵ��� 
	//��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	//��� ����� ExecuteCommandList�� ���ؼ� ��� ��⿭�� 
	//�߰��ߴٸ� ��� ����� �缳���� �� �ִ�. ��� ����� 
	//�缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//�ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�ĸ� ������ ���� ���۸� �����.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), 
		Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.f, 0, 0, nullptr);

	//������ ����� ��ϵ� ���� ��� ���۵��� �����Ѵ�.
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

	//�ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//��ɵ��� ����� ��ģ��.
	ThrowIfFailed(mCommandList->Close());

	//��� ������ ���� ��� ����� ��� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//�ĸ� ���ۿ� ���� ���۸� ��ȯ�Ѵ�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//�� �������� ��ɵ��� ��� ó���Ǳ� ��ٸ���. �̷��� ���� 
	//��ȿ�����̴�. �̹����� ������ �������� ���� �� ����� ���������, 
	//������ �����鿡���� ������ �ڵ带 ������ ����ȭ�ؼ� �����Ӹ��� ����� 
	//�ʿ䰡 ���� �����.
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
		//���콺 �� �ȼ� �̵��� 4���� 1���� ������Ų��.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		//���콺 �Է¿� ������ ������ �����Ѵ�. �̿� ���� ī�޶� ���ڸ� �߽����� 
		//�����ϰ� �ȴ�.
		mTheta += dx;
		mPhi += dy;

		//mPhi ������ �����Ѵ�.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if((btnState & MK_RBUTTON) != 0) {
		//���콺 �� �ȼ� �̵��� ����� 0.005������ ������Ų��.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		//���콺 �Է¿� �����ؼ� ī�޶� �������� �����Ѵ�.
		mRadius += dx - dy;

		//�������� �����Ѵ�.
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
	//���ۿ��� i��° ��ü�� ��� ������ �������� ��´�.
	//������ i = 0�̴�.
	int boxCBufIndex = 0;
	cbAddress += boxCBufIndex * objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature() {
	//�Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)�� 
	//�Էµȴٰ� ����Ѵ�. ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� 
	//�����Ѵ�. ���̴� ���α׷��� ���������� �ϳ��� �Լ��̰� ���̴��� �ԷµǴ� 
	//�ڿ����� �Լ��� �Ű������鿡 �ش��ϹǷ�, ��Ʈ ������ �� �Լ� ������ �����ϴ� 
	//�����̶� �� �� �ִ�.

	//��Ʈ �Ű������� ������ ���̺��̰ų� ��Ʈ ������ �Ǵ� ��Ʈ ����̴�.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	//CBV �ϳ��� ��� ������ ���̺��� �����Ѵ�.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	//��Ʈ ������ ��Ʈ �Ű��������� �迭�̴�.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//��� ���� �ϳ��� ������ ������ ������ ����Ű�� 
	//���� �ϳ��� �̷���� ��Ʈ ������ �����Ѵ�.
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
		//�ո�
		0, 1, 2,
		0, 2, 3,
	
		//�޸�
		4, 6, 5,
		4, 7, 6,
	
		//���� ��
		4, 5, 1,
		4, 1, 0,
	
		//������ ��
		3, 2, 6,
		3, 6, 7,
	
		//����
		1, 5, 6,
		1, 6, 2,
	
		//�Ʒ���
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
