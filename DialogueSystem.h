#pragma once
#include "KamataEngine.h"
#include <cstdint>
#include <string>
#include <vector>

class DialogueSystem {
public:
	~DialogueSystem();

	void Initialize(uint32_t pageCount, const std::vector<std::string>& spriteFiles = {}, float screenHeightRatio = 0.25f);
	void Start();
	void Update();
	void Draw() const;
	bool IsActive() const;
	bool IsFinished() const { return phase_ == Phase::kFinished; }

private:
	enum class Phase { kIdle, kAppearing, kWaiting, kDisappearing, kFinished };

	void ApplyCurrentPageVisual(float visibility);
	static float SmoothStep(float t);

	static inline const float kFrameTime = 1.0f / 60.0f;
	static inline const float kAppearDuration = 0.38f;
	static inline const float kDisappearDuration = 0.30f;
	static inline const float kSlideDistance = 64.0f;

	std::vector<KamataEngine::Sprite*> pageSprites_;
	std::vector<KamataEngine::Vector4> pageBaseColors_;
	Phase phase_ = Phase::kIdle;
	uint32_t currentPage_ = 0;
	float phaseTimer_ = 0.0f;
	float screenHeightRatio_ = 0.25f;
};
