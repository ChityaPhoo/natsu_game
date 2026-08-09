#pragma once
#include "KamataEngine.h"
#include <cstdint>
#include <string>
#include <vector>

class MapChipField {
public:
	static inline const uint32_t kInvalidIndex = static_cast<uint32_t>(-1);
	enum class MapChipType : uint32_t { kBlank = 0, kBlock };
	struct IndexSet { uint32_t xIndex = kInvalidIndex; uint32_t yIndex = kInvalidIndex; };
	struct Rect { float left = 0.0f; float right = 0.0f; float bottom = 0.0f; float top = 0.0f; };

	~MapChipField();
	void Initialize(const std::string& objectName, const std::string& csvPath, float chipWidth, float chipHeight);
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	uint32_t GetMapChipCountHorizontal() const;
	uint32_t GetMapChipCountVertical() const;
	MapChipType GetMapChipTypeFromIndex(uint32_t xIndex, uint32_t yIndex) const;
	KamataEngine::Vector3 GetMapChipPositionFromIndex(uint32_t xIndex, uint32_t yIndex) const;
	IndexSet GetIndexFromMapChipPosition(const KamataEngine::Vector3& position) const;
	Rect GetRectFromIndex(uint32_t xIndex, uint32_t yIndex) const;

private:
	void LoadCsv(const std::string& csvPath);
	void BuildWorld();
	void ClearWorld();
	KamataEngine::Model* mapChipModel_ = nullptr;
	std::vector<std::vector<MapChipType>> mapData_;
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransforms_;
	float chipWidth_ = 1.0f;
	float chipHeight_ = 1.0f;
};
