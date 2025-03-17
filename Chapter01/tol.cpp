#include <Windows.h> //XMVerifyCPUSupport에 필요함
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

int main() {
	cout.precision(8);

	//SSE2를 지원하는지(Pentium4, AMD K8 이상) 확인한다.
	if (!XMVerifyCPUSupport()) {
		cout << "DirectXMath를 지원하지 않음" << endl;
		return 0;
	}

	XMVECTOR u = XMVectorSet(1.f, 1.f, 1.f, 0.f);
	XMVECTOR n = XMVector3Normalize(u);

	float LU = XMVectorGetX(XMVector3Length(n));

	//수학적으로 길이가 반드시 1이어야 한다. 수치적으로도 그럴까?
	cout << LU << endl;
	if (LU == 1.f) cout << "길이 1" << endl;
	else cout << "길이 1 아님" << endl;

	//1을 임의의 지수로 거듭제곱해도 여전히 1이어야 한다. 실제로 그럴까?
	float powLU = powf(LU, 1.e6f);
	cout << "LU^(10^6) = " << powLU << endl;
}
