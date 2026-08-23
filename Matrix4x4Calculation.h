#pragma once
#include "KamataEngine.h"

class Matrix4x4Calculation {
public:
	static KamataEngine::Matrix4x4 MakeIdentity4x4();
	static KamataEngine::Matrix4x4 Inverse(const KamataEngine::Matrix4x4& matrix);
	static KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& lhs, const KamataEngine::Matrix4x4& rhs);
	static KamataEngine::Matrix4x4 MakeTranslateMatrix(const KamataEngine::Vector3& translation);
	static KamataEngine::Matrix4x4 MakeScaleMatrix(const KamataEngine::Vector3& scale);
	static KamataEngine::Matrix4x4 MakeRotateXMatrix(float radians);
	static KamataEngine::Matrix4x4 MakeRotateYMatrix(float radians);
	static KamataEngine::Matrix4x4 MakeRotateZMatrix(float radians);
	static KamataEngine::Matrix4x4 MakeAffineMatrix(
	    const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotation, const KamataEngine::Vector3& translation);
	static KamataEngine::Vector3 TransformPoint(const KamataEngine::Vector3& point, const KamataEngine::Matrix4x4& matrix);
};
