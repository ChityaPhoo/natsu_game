#pragma once
#include "KamataEngine.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class DialogueSystem {
public:
	~DialogueSystem();

	void Initialize(
	    uint32_t pageCount,
	    const std::vector<std::string>& spriteFiles = {},
	    float screenHeightRatio = 0.25f,
	    KamataEngine::Vector2 textureCropBase = {},
	    KamataEngine::Vector2 textureCropSize = {});
	void Start();
	void Update();
	void Draw() const;
	void SetPageContentSprites(
	    const std::vector<std::string>& spriteFiles,
	    KamataEngine::Vector2 displaySize,
	    KamataEngine::Vector2 offset = {});
	void SetOpacity(float opacity);
	void SetBaseColor(const KamataEngine::Vector4& color);
	bool IsActive() const;
	bool IsFinished() const { return phase_ == Phase::kFinished; }
	uint32_t GetCurrentPage() const { return currentPage_; }
	float GetCurrentVisibility() const { return currentVisibility_; }
	float GetCurrentSlideOffset() const { return kSlideDistance * (1.0f - currentVisibility_); }

private:
	enum class Phase { kIdle, kAppearing, kWaiting, kDisappearing, kFinished };

	void ApplyCurrentPageVisual(float visibility);
	void UpdateAdvanceIndicatorVisual();
	static float SmoothStep(float t);

	static inline const float kFrameTime = 1.0f / 60.0f;
	static inline const float kAppearDuration = 0.38f;
	static inline const float kDisappearDuration = 0.30f;
	static inline const float kSlideDistance = 64.0f;
	static inline constexpr std::size_t kAdvanceIndicatorCount = 3;
	static inline const float kAdvanceIndicatorSize = 10.0f;
	static inline const float kAdvanceIndicatorSpacing = 16.0f;
	static inline const float kAdvanceIndicatorRightPadding = 48.0f;
	static inline const float kAdvanceIndicatorBottomPadding = 28.0f;
	static inline const float kAdvanceIndicatorMoveAmount = 6.0f;
	static inline const float kAdvanceIndicatorCycleDuration = 0.85f;

	std::vector<KamataEngine::Sprite*> pageSprites_;
	std::vector<KamataEngine::Vector4> pageBaseColors_;
	std::vector<KamataEngine::Sprite*> contentSprites_;
	std::array<KamataEngine::Sprite*, kAdvanceIndicatorCount> advanceIndicators_ = {};
	KamataEngine::Vector2 contentDisplaySize_ = {};
	KamataEngine::Vector2 contentOffset_ = {};
	Phase phase_ = Phase::kIdle;
	uint32_t currentPage_ = 0;
	float phaseTimer_ = 0.0f;
	float screenHeightRatio_ = 0.25f;
	float opacity_ = 1.0f;
	float currentVisibility_ = 0.0f;
	float advanceIndicatorTimer_ = 0.0f;
};
