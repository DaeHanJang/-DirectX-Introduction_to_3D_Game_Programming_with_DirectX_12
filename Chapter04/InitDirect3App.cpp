#include "../Common/d3dApp.h"
#include <DirectXColors.h>

using namespace DirectX;

class InitDirect3DApp : public D3DApp {
public:
	InitDirect3DApp(HINSTANCE hInstance);
	~InitDirect3DApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	//디버그 빌드에서는 실행지점 메모리 점검 기능을 켠다.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try {
		InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize()) return 0;

		return theApp.Run();
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

InitDirect3DApp::~InitDirect3DApp() {}

bool InitDirect3DApp::Initialize() {
	if (!D3DApp::Initialize()) return false;
	return true;
}

void InitDirect3DApp::OnResize() {
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt) {}

void InitDirect3DApp::Draw(const GameTimer& gt) {
	//명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를 재설정한다.
	//재설정은 GPU가 관련 명령 목록들을 
	//모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	//명령 목록을 ExecuteCommandList를 통해서 명령 대기열에 
	//추가했다면 명령 목록을 재설정할 수 있다. 명령 목록을 
	//재설정하면 메모리가 재활용된다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	//자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(), 
			D3D12_RESOURCE_STATE_PRESENT, 
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	//뷰포트와 가위 직사각형을 설정한다.
	//명령 목록을 재설정할 때마다 이들도 재설정해야 한다.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//후면 버퍼와 깊이 버퍼를 지운다.
	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(), 
		Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(
		DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | 
		D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	//렌더링 결과가 기록될 렌더 대상 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), 
		true, &DepthStencilView());

	//자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(), 
			D3D12_RESOURCE_STATE_RENDER_TARGET, 
			D3D12_RESOURCE_STATE_PRESENT));

	//명령들의 기록을 마친다.
	ThrowIfFailed(mCommandList->Close());

	//명령 실행을 위해 명령 목록을 명령 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//후면 보퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//이 프레임의 명령들이 모두 처리되길 기다린다. 이런한 대기는 
	//비효율적이다. 이번에는 예제의 간단함을 위해 이 방법을 사용하지만, 
	//이후의 예제들에서는 렌더링 코드를 적절히 조직화해서 프레임마다 대기할 
	//필요가 없게 만든다.
	FlushCommandQueue();
}
