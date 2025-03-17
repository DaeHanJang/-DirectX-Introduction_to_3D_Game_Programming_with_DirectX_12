#include <Windows.h> //XMVertifyCPUSupport에 필요함
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

//XMVECTOR 객체와 XMMATRIX 객체를 cout으로 출력할 수 있도록
//<< 연산자를 중복적재한다.
ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v) {
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";
	return os;
}

ostream& XM_CALLCONV operator << (ostream& os, FXMMATRIX m) {
	for (int i = 0; i < 4; i++) {
		os << XMVectorGetX(m.r[i]) << "\t";
		os << XMVectorGetY(m.r[i]) << "\t";
		os << XMVectorGetZ(m.r[i]) << "\t";
		os << XMVectorGetW(m.r[i]);
		os << endl;
	}
	return os;
}

template<size_t Row, size_t Col>
void PrintTranspose(float (&m)[Row][Col]) {
	for (int j = 0; j < Col; j++) {
		for (int i = 0; i < Row; i++) cout << m[i][j] << " ";
		cout << endl;
	}
}

float Determinant(float m[][4], int n) {
	float result = 0;

	if (n <= 2) result = m[0][0] * m[1][1] - m[0][1] * m[1][0];
	else {
		for (int i = 0; i < n; i++) {
			float temp[4][4];
			for (int r = 0; r < n - 1; r++) {
				for (int j = 0, c = 0; j < n; j++) {
					if (i == j) continue;
					temp[r][c] = m[r + 1][j];
					c++;
				}
			}
			if (i % 2 == 0) result += m[0][i] * Determinant(temp, n - 1);
			else result -= m[0][i] * Determinant(temp, n - 1);
		}
	}

	return result;
}

void PrintInverse(float m[][4]) {
	float C[4][4];
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			float temp[4][4];
			for (int k = 0, r = 0; k < 4; k++) {
				if (k == i) continue;
				for (int l = 0, c = 0; l < 4; l++) {
					if (l == j) continue;
					temp[r][c] = m[k][l];
					c++;
				}
				r++;
			}
			C[i][j] = ((i + j) % 2 == 0) ? Determinant(temp, 3) : -Determinant(temp, 3);
		}
	}
	float det = Determinant(m, 4);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) cout << C[j][i] / det << " ";
		cout << endl;
	}
}

int main() {
	//SSE2를 지원하는지(Pentium4, AMD K8 이상) 확인한다.
	if (!XMVerifyCPUSupport()) {
		cout << "DirectXMath를 지원하지 않음" << endl;
		return 0;
	}

	XMMATRIX A(1.f, 0.f, 0.f, 0.f,
		0.f, 2.f, 0.f, 0.f,
		0.f, 0.f, 4.f, 0.f,
		1.f, 2.f, 3.f, 1.f);
	XMMATRIX B = XMMatrixIdentity();
	XMMATRIX C = A * B;
	XMMATRIX D = XMMatrixTranspose(A);
	XMVECTOR det = XMMatrixDeterminant(A);
	XMMATRIX E = XMMatrixInverse(&det, A);
	XMMATRIX F = A * E;

	float m[4][4] = { { 1.f, 0.f, 0.f, 0.f },
		{ 0.f, 2.f, 0.f, 0.f },
		{ 0.f, 0.f, 4.f, 0.f },
		{ 1.f, 2.f, 3.f, 1.f } };

	cout << "A = " << endl << A << endl;
	cout << "B = " << endl << B << endl;
	cout << "C = A*B = " << endl << C << endl;
	cout << "D = transpose(A) = " << endl << D << endl;
	cout << "det = determinant(A) = " << det << endl << endl;
	cout << "E = inverse(A) = " << endl << E << endl;
	cout << "F = A*E = " << endl << F << endl;

	PrintTranspose(m);
	cout << Determinant(m, 4) << endl;
	PrintInverse(m);

	return 0;
}
