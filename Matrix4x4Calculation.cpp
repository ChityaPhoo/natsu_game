#include "Matrix4x4Calculation.h"
#include <cmath>

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeIdentity4x4() {
	KamataEngine::Matrix4x4 result = {};
	for (int row = 0; row < 4; ++row) { result.m[row][row] = 1.0f; }
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::Multiply(const KamataEngine::Matrix4x4& lhs, const KamataEngine::Matrix4x4& rhs) {
	KamataEngine::Matrix4x4 result = {};
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			for (int index = 0; index < 4; ++index) { result.m[row][column] += lhs.m[row][index] * rhs.m[index][column]; }
		}
	}
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeTranslateMatrix(const KamataEngine::Vector3& translation) {
	auto result = MakeIdentity4x4();
	result.m[3][0] = translation.x; result.m[3][1] = translation.y; result.m[3][2] = translation.z;
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeScaleMatrix(const KamataEngine::Vector3& scale) {
	KamataEngine::Matrix4x4 result = {};
	result.m[0][0] = scale.x; result.m[1][1] = scale.y; result.m[2][2] = scale.z; result.m[3][3] = 1.0f;
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeRotateXMatrix(float radians) {
	auto result = MakeIdentity4x4();
	result.m[1][1] = std::cos(radians); result.m[1][2] = std::sin(radians);
	result.m[2][1] = -std::sin(radians); result.m[2][2] = std::cos(radians);
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeRotateYMatrix(float radians) {
	auto result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radians); result.m[0][2] = -std::sin(radians);
	result.m[2][0] = std::sin(radians); result.m[2][2] = std::cos(radians);
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeRotateZMatrix(float radians) {
	auto result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radians); result.m[0][1] = std::sin(radians);
	result.m[1][0] = -std::sin(radians); result.m[1][1] = std::cos(radians);
	return result;
}

KamataEngine::Matrix4x4 Matrix4x4Calculation::MakeAffineMatrix(
	const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotation, const KamataEngine::Vector3& translation) {
	const auto rotate = Multiply(MakeRotateXMatrix(rotation.x), Multiply(MakeRotateYMatrix(rotation.y), MakeRotateZMatrix(rotation.z)));
	return Multiply(MakeScaleMatrix(scale), Multiply(rotate, MakeTranslateMatrix(translation)));
}

KamataEngine::Vector3 Matrix4x4Calculation::TransformPoint(
	const KamataEngine::Vector3& point, const KamataEngine::Matrix4x4& matrix) {
	KamataEngine::Vector3 result = {
	    point.x * matrix.m[0][0] + point.y * matrix.m[1][0] + point.z * matrix.m[2][0] + matrix.m[3][0],
	    point.x * matrix.m[0][1] + point.y * matrix.m[1][1] + point.z * matrix.m[2][1] + matrix.m[3][1],
	    point.x * matrix.m[0][2] + point.y * matrix.m[1][2] + point.z * matrix.m[2][2] + matrix.m[3][2],
	};
	const float w = point.x * matrix.m[0][3] + point.y * matrix.m[1][3] + point.z * matrix.m[2][3] + matrix.m[3][3];
	if (w != 0.0f) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}
	return result;
}
