#pragma once
#include "KamataEngine.h"

class Player;

class CameraController {
public:
	struct Rect { float left = 0.0f; float right = 1.0f; float bottom = 0.0f; float top = 1.0f; };
	void Initialize();
	void ResetCameraPosition();
	void Update();
	void LockToPosition(const KamataEngine::Vector3& position, float duration);
	void Unlock();
	bool IsLocked() const { return isLocked_; }
	bool IsLockComplete() const { return isLocked_ && !isLockTransitioning_; }
	void SetPlayer(Player* player) { target_ = player; }
	void SetMovableArea(const Rect& rect) { movableArea_ = rect; }
	const KamataEngine::Camera& GetCamera() const { return camera_; }
	KamataEngine::Camera& GetCamera() { return camera_; }

private:
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
};
