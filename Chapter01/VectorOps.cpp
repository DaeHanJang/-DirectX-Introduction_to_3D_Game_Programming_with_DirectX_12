#include <Windows.h> //XMVerifyCPUSupport에 필요함
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

//XMVECTOR 객체를 cout으로 출력하기 위해
//"<<" 연산자를 중복적재한다.
ostream& XM_CALLCONV operator<<(ostream& os, FXMVECTOR v) {
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";

	return os;
}

int main() {
	cout.setf(ios_base::boolalpha);

	//SSE2를 지원하는지(Pentium4, AMD K8 이상) 확인한다.
	if (!XMVerifyCPUSupport()) {
		cout << "DirectXMath를 지원하지 않음" << endl;
		return 0;
	}

	XMVECTOR p = XMVectorSet(2.f, 2.f, 1.f, 0.f);
	XMVECTOR q = XMVectorSet(2.f, -0.5f, 0.5f, 0.1f);
	XMVECTOR u = XMVectorSet(1.f, 2.f, 4.f, 8.f);
	XMVECTOR v = XMVectorSet(-2.f, 1.f, -3.f, 2.5f);
	XMVECTOR w = XMVectorSet(0.f, XM_PIDIV4, XM_PIDIV2, XM_PI);

	cout << "XMVectorAbs(v) = " << XMVectorAbs(v) << endl;
	cout << "XMVectorCos(w) = " << XMVectorCos(w) << endl;
	cout << "XMVectorLog(u) = " << XMVectorLog(u) << endl;
	cout << "XMVectorExp(p) = " << XMVectorExp(p) << endl;

	cout << "XMVectorPow(u, p) = " << XMVectorPow(u, p) << endl;
	cout << "XMVectorSqrt(u) = " << XMVectorSqrt(u) << endl;

	cout << "XMVectorSwizzle(u, 2, 2, 1, 3) = " << XMVectorSwizzle(u, 2, 2, 1, 3) << endl;
	cout << "XMVectorSwizzle(u, 2, 1, 0, 3) = " << XMVectorSwizzle(u, 2, 1, 0, 3) << endl;

	cout << "XMVectorMultiply(u, v) = " << XMVectorMultiply(u, v) << endl;
	cout << "XMVectorSaturate(q) = " << XMVectorSaturate(q) << endl;
	cout << "XMVectorMin(p, v) = " << XMVectorMin(p, v) << endl;
	cout << "XMVectorMax(p, v) = " << XMVectorMax(p, v) << endl;

	return 0;
}
