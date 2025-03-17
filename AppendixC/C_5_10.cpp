#include <Windows.h> //FLOAT ���Ǹ� ����
#include <DirectXMath.h>
#include <iostream>

using namespace std;
using namespace DirectX;

//XMVECTOR ��ü�� cout���� ����ϱ� ����
//<< �����ڸ� �ߺ������Ѵ�.
ostream& XM_CALLCONV operator<<(ostream& os, FXMVECTOR v) {
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";

	return os;
}

int main() {
	XMVECTOR p0 = XMVectorSet(-1.f, 1.f, -1.f, 1.f);
	XMVECTOR u = XMVectorSet(1.f, 0.f, 0.f, 0.f);

	//��� �������� (A, B, C, D) ������ ���� �����ؼ� 
	//����� �����Ѵ�.
	float s = 1.f / sqrtf(3.f);
	XMVECTOR plane = XMVectorSet(s, s, s, -5.f);

	//�� �Լ��� �������� �ƴ϶� ������ �޴´�. �׷��� �������� 
	//p0 + 100 * u���� �߶� ������ �����.
	XMVECTOR isect = XMPlaneIntersectLine(plane, p0, p0 + 100 * u);
	cout << isect << endl;

	return 0;
}
