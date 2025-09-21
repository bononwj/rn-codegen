#pragma once

#include "Types.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace facebook::react {

class BurstDetectionEngine {
public:
  explicit BurstDetectionEngine(
      const BurstDetectionConfig &config = BurstDetectionConfig());

  // 检测连拍分组
  BurstDetectionResult detectBursts(const std::vector<PhotoInfo> &photos);

  // 更新配置
  void updateConfig(const BurstDetectionConfig &config);

  // 获取当前配置
  const BurstDetectionConfig &getConfig() const { return config_; }

private:
  BurstDetectionConfig config_;

  // 核心算法方法
  std::vector<BurstGroup>
  analyzeBursts(const std::vector<PhotoInfo> &sortedPhotos);

  // 多层级判断方法
  std::vector<std::vector<size_t>>
  findTimeBasedCandidates(const std::vector<PhotoInfo> &photos);
  std::vector<std::vector<size_t>>
  refineByTimingAnalysis(const std::vector<PhotoInfo> &photos,
                         const std::vector<std::vector<size_t>> &candidates);
  std::vector<std::vector<size_t>>
  validateByExifInfo(const std::vector<PhotoInfo> &photos,
                     const std::vector<std::vector<size_t>> &candidates);
  std::vector<std::vector<size_t>>
  checkFileNamePatterns(const std::vector<PhotoInfo> &photos,
                        const std::vector<std::vector<size_t>> &candidates);

  // 工具方法
  bool arePhotosCompatible(const PhotoInfo &photo1,
                           const PhotoInfo &photo2) const;
  bool isTimeGapValid(int64_t gap) const;
  bool areTimingIntervalsStable(const std::vector<int64_t> &intervals) const;
  bool areExifInfoSimilar(const PhotoInfo &photo1,
                          const PhotoInfo &photo2) const;
  bool areFileNamesSequential(const std::string &name1,
                              const std::string &name2) const;
  double calculateGpsDistance(const PhotoInfo &photo1,
                              const PhotoInfo &photo2) const;

  // 分组创建方法
  BurstGroup createBurstGroup(const std::vector<PhotoInfo> &photos,
                              const std::vector<size_t> &photoIndices);
  std::string generateGroupId() const;
  std::string
  selectRepresentativePhoto(const std::vector<std::string> &photoIds) const;

  // 排序和过滤
  std::vector<PhotoInfo>
  sortPhotosByTimestamp(const std::vector<PhotoInfo> &photos) const;
  void filterSmallGroups(std::vector<BurstGroup> &groups) const;

  // 统计方法
  void calculateStatistics(BurstDetectionResult &result) const;

  // 性能计时
  int64_t getCurrentTimeMs() const;
};

} // namespace facebook::react
