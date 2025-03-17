#include <Windows.h> //FLOAT 정의를 위해
#include <DirectXMath.h>
#include <iostream>

using namespace std;
using namespace DirectX;

//XMVECTOR 객체를 cout으로 출력하기 위해
//<< 연산자를 중복적재한다.
ostream& XM_CALLCONV operator<<(ostream& os, FXMVECTOR v) {
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";

	return os;
}

int main() {
	XMVECTOR p0 = XMVectorSet(-1.f, 1.f, -1.f, 1.f);
	XMVECTOR u = XMVectorSet(1.f, 0.f, 0.f, 0.f);

	//평면 방정식의 (A, B, C, D) 성분을 직접 지정해서 
	//평면을 구축한다.
	float s = 1.f / sqrtf(3.f);
	XMVECTOR plane = XMVectorSet(s, s, s, -5.f);

	//이 함수는 반직선이 아니라 선분을 받는다. 그래서 반직선을 
	//p0 + 100 * u에서 잘라서 선분을 만든다.
	XMVECTOR isect = XMPlaneIntersectLine(plane, p0, p0 + 100 * u);
	cout << isect << endl;

	return 0;
}
