#include "MapChipField.h"
#include "Matrix4x4Calculation.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace KamataEngine;

void MapChipField::Initialize(const std::string& objectName, const std::string& csvPath, float chipWidth, float chipHeight) {
	chipWidth_ = chipWidth; chipHeight_ = chipHeight;
	mapChipModel_ = Model::CreateFromOBJ(objectName.c_str(), true);
	LoadCsv(csvPath); BuildWorld();
}

void MapChipField::LoadCsv(const std::string& csvPath) {
	mapData_.clear();
	std::ifstream file(csvPath);
	if (!file.is_open()) { return; }
	std::string line; size_t columnCount = 0;
	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r') { line.pop_back(); }
		if (line.empty()) { continue; }
		std::stringstream stream(line); std::string cell; std::vector<MapChipType> row;
		while (std::getline(stream, cell, ',')) {
			cell.erase(std::remove_if(cell.begin(), cell.end(), [](unsigned char c) { return std::isspace(c) != 0; }), cell.end());
			const bool isBlock = cell == "1" || cell == "B" || cell == "B0" || cell == "B1";
			row.push_back(isBlock ? MapChipType::kBlock : MapChipType::kBlank);
		}
		columnCount = (std::max)(columnCount, row.size()); mapData_.push_back(row);
	}
	for (auto& row : mapData_) { row.resize(columnCount, MapChipType::kBlank); }
}

void MapChipField::Update() {
	for (const auto& row : worldTransforms_) {
		for (WorldTransform* transform : row) {
			if (transform == nullptr) { continue; }
			transform->matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(transform->scale_, transform->rotation_, transform->translation_);
			transform->TransferMatrix();
		}
	}
}

void MapChipField::Draw(const Camera& camera) {
	if (mapChipModel_ == nullptr) { return; }
	for (const auto& row : worldTransforms_) { for (WorldTransform* transform : row) { if (transform != nullptr) { mapChipModel_->Draw(*transform, camera); } } }
}

uint32_t MapChipField::GetMapChipCountHorizontal() const { return mapData_.empty() ? 0u : static_cast<uint32_t>(mapData_.front().size()); }
uint32_t MapChipField::GetMapChipCountVertical() const { return static_cast<uint32_t>(mapData_.size()); }

MapChipField::MapChipType MapChipField::GetMapChipTypeFromIndex(uint32_t xIndex, uint32_t yIndex) const {
	if (xIndex >= GetMapChipCountHorizontal() || yIndex >= GetMapChipCountVertical()) { return MapChipType::kBlank; }
	return mapData_[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionFromIndex(uint32_t xIndex, uint32_t yIndex) const {
	const uint32_t rows = GetMapChipCountVertical();
	if (rows == 0) { return {}; }
	return {chipWidth_ * (static_cast<float>(xIndex) + 0.5f), chipHeight_ * (static_cast<float>((rows - 1u) - yIndex) + 0.5f), 0.0f};
}

MapChipField::IndexSet MapChipField::GetIndexFromMapChipPosition(const Vector3& position) const {
	IndexSet result = {}; const uint32_t rows = GetMapChipCountVertical(); const uint32_t columns = GetMapChipCountHorizontal();
	if (rows == 0 || columns == 0 || chipWidth_ <= 0.0f || chipHeight_ <= 0.0f || position.x < 0.0f || position.y < 0.0f) { return result; }
	const uint32_t x = static_cast<uint32_t>(std::floor(position.x / chipWidth_));
	const uint32_t yFromBottom = static_cast<uint32_t>(std::floor(position.y / chipHeight_));
	if (x >= columns || yFromBottom >= rows) { return result; }
	result.xIndex = x; result.yIndex = (rows - 1u) - yFromBottom; return result;
}

MapChipField::Rect MapChipField::GetRectFromIndex(uint32_t xIndex, uint32_t yIndex) const {
	const Vector3 center = GetMapChipPositionFromIndex(xIndex, yIndex);
	return {center.x - chipWidth_ * 0.5f, center.x + chipWidth_ * 0.5f, center.y - chipHeight_ * 0.5f, center.y + chipHeight_ * 0.5f};
}

void MapChipField::BuildWorld() {
	ClearWorld(); const uint32_t rows = GetMapChipCountVertical(); const uint32_t columns = GetMapChipCountHorizontal(); worldTransforms_.resize(rows);
	for (uint32_t y = 0; y < rows; ++y) {
		worldTransforms_[y].resize(columns, nullptr);
		for (uint32_t x = 0; x < columns; ++x) {
			if (GetMapChipTypeFromIndex(x, y) != MapChipType::kBlock) { continue; }
			auto* transform = new WorldTransform(); transform->Initialize(); transform->translation_ = GetMapChipPositionFromIndex(x, y); worldTransforms_[y][x] = transform;
		}
	}
}

void MapChipField::ClearWorld() {
	for (auto& row : worldTransforms_) { for (WorldTransform*& transform : row) { delete transform; transform = nullptr; } }
	worldTransforms_.clear();
}

MapChipField::~MapChipField() { ClearWorld(); delete mapChipModel_; mapChipModel_ = nullptr; }
