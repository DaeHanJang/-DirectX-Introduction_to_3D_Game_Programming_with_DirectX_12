#include <Windows.h> //XMVerifyCPUSupport�� �ʿ���
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

int main() {
	cout.precision(8);

	//SSE2�� �����ϴ���(Pentium4, AMD K8 �̻�) Ȯ���Ѵ�.
	if (!XMVerifyCPUSupport()) {
		cout << "DirectXMath�� �������� ����" << endl;
		return 0;
	}

	XMVECTOR u = XMVectorSet(1.f, 1.f, 1.f, 0.f);
	XMVECTOR n = XMVector3Normalize(u);

	float LU = XMVectorGetX(XMVector3Length(n));

	//���������� ���̰� �ݵ�� 1�̾�� �Ѵ�. ��ġ�����ε� �׷���?
	cout << LU << endl;
	if (LU == 1.f) cout << "���� 1" << endl;
	else cout << "���� 1 �ƴ�" << endl;

	//1�� ������ ������ �ŵ������ص� ������ 1�̾�� �Ѵ�. ������ �׷���?
	float powLU = powf(LU, 1.e6f);
	cout << "LU^(10^6) = " << powLU << endl;
}
