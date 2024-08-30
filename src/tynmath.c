#include "tynmath.h"

float dlerp(float a, float b, float decay, float dt) {
	return b + (a - b) * expf(-decay * dt);
}

Vector3 Vector3RotateByMatrix(Vector3 v, Matrix m) {
	v = Vector3Transform(v, m);

	// extract position
	Vector3 pos = Vector3Transform(Vector3Zero(), m);
	v = Vector3Subtract(v, pos);

	return v;
}

Matrix MatrixJustRotate(Vector3 r, Matrix m) {
	Vector3 pos = Vector3Transform(Vector3Zero(), m);
	// 1. discard origin
	m = MatrixMultiply(m, MatrixTranslate(-pos.x, -pos.y, -pos.z));
	// 2. rotate
	m = MatrixMultiply(m, MatrixRotateXYZ(r));
	// 3. translate back
	m = MatrixMultiply(m, MatrixTranslate(pos.x, pos.y, pos.z));

	return m;
}
