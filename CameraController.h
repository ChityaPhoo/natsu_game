#pragma once
#include "KamataEngine.h"

class Player;

class CameraController {
public:
	struct Rect { float left = 0.0f; float right = 1.0f; float bottom = 0.0f; float top = 1.0f; };
	void Initialize();
	void ResetCameraPosition();
	void Update();
	void StartShake(float duration, float intensity);
	void LockToPosition(const KamataEngine::Vector3& position, float duration);
	void Unlock();
	void SetDebugMode(bool active, const KamataEngine::Vector3& focusPoint);
	bool IsLocked() const { return isLocked_; }
	bool IsLockComplete() const { return isLocked_ && !isLockTransitioning_; }
	void SetPlayer(Player* player) { target_ = player; }
	void SetMovableArea(const Rect& rect) { movableArea_ = rect; }
	const KamataEngine::Camera& GetCamera() const { return camera_; }
	KamataEngine::Camera& GetCamera() { return camera_; }

private:
	void UpdateDebugCamera();
	void RemoveShakeOffset();
	void ApplyShake();
	static KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& start, const KamataEngine::Vector3& end, float t);
	static inline const float kInterpolationRate = 0.10f;
	static inline const float kVelocityBias = 2.0f;
	static inline const Rect kTargetMargin = {-9.0f, 9.0f, -5.0f, 5.0f};
	KamataEngine::Camera camera_;
	KamataEngine::Vector3 offset_ = {0.0f, 0.0f, -15.0f};
	Rect movableArea_ = {};
	Player* target_ = nullptr;
	KamataEngine::Vector3 lockStartPosition_ = {};
	KamataEngine::Vector3 lockedPosition_ = {};
	bool isLocked_ = false;
	bool isLockTransitioning_ = false;
	float lockTimer_ = 0.0f;
	float lockDuration_ = 0.0f;
	KamataEngine::Vector3 debugFocusPoint_ = {};
	KamataEngine::Vector3 debugEntryTranslation_ = {};
	KamataEngine::Vector3 debugEntryRotation_ = {};
	float debugYaw_ = 0.0f;
	float debugPitch_ = 0.0f;
	float debugDistance_ = 15.0f;
	bool isDebugMode_ = false;
	KamataEngine::Vector3 shakeOffset_ = {};
	float shakeTimer_ = 0.0f;
	float shakeDuration_ = 0.0f;
	float shakeIntensity_ = 0.0f;
	float shakePhase_ = 0.0f;
	static inline const float kDebugRotateSpeed = 0.005f;
	static inline const float kDebugZoomSpeed = 0.010f;
	static inline const float kDebugPanSpeed = 0.0015f;
	static inline const float kDebugMinDistance = 2.0f;
	static inline const float kDebugMaxDistance = 80.0f;
};
