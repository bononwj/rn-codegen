#include "BurstDetectionEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <random>
#include <regex>
#include <sstream>

namespace facebook::react {

BurstDetectionEngine::BurstDetectionEngine(const BurstDetectionConfig &config)
    : config_(config) {}

BurstDetectionResult
BurstDetectionEngine::detectBursts(const std::vector<PhotoInfo> &photos) {
  auto startTime = getCurrentTimeMs();
  BurstDetectionResult result;

  if (photos.empty()) {
    result.processingTimeMs = getCurrentTimeMs() - startTime;
    return result;
  }

  // 1. 按时间戳排序
  auto sortedPhotos = sortPhotosByTimestamp(photos);

  // 2. 分析连拍
  auto groups = analyzeBursts(sortedPhotos);

  // 3. 过滤小分组
  filterSmallGroups(groups);

  // 4. 计算统计信息
  result.groups = std::move(groups);
  calculateStatistics(result);
  result.processingTimeMs = getCurrentTimeMs() - startTime;

  return result;
}

void BurstDetectionEngine::updateConfig(const BurstDetectionConfig &config) {
  config_ = config;
}

std::vector<BurstGroup> BurstDetectionEngine::analyzeBursts(
    const std::vector<PhotoInfo> &sortedPhotos) {
  std::vector<BurstGroup> groups;

  // 第一层：基于时间窗口的候选分组
  auto timeCandidates = findTimeBasedCandidates(sortedPhotos);

  // 第二层：精确时间分析
  auto timingRefined = refineByTimingAnalysis(sortedPhotos, timeCandidates);

  // 第三层：EXIF信息验证
  auto exifValidated = validateByExifInfo(sortedPhotos, timingRefined);

  // 第四层：文件名模式检查
  auto finalCandidates = checkFileNamePatterns(sortedPhotos, exifValidated);

  // 创建分组
  for (const auto &candidate : finalCandidates) {
    if (candidate.size() >= static_cast<size_t>(config_.minBurstSize)) {
      auto group = createBurstGroup(sortedPhotos, candidate);
      group.type = GroupType::BURST;
      groups.push_back(std::move(group));
    }
  }

  // 为未分组的照片创建单张分组
  std::vector<bool> assigned(sortedPhotos.size(), false);
  for (const auto &candidate : finalCandidates) {
    for (size_t idx : candidate) {
      assigned[idx] = true;
    }
  }

  for (size_t i = 0; i < sortedPhotos.size(); ++i) {
    if (!assigned[i]) {
      auto group = createBurstGroup(sortedPhotos, {i});
      group.type = GroupType::SINGLE;
      groups.push_back(std::move(group));
    }
  }

  return groups;
}

std::vector<std::vector<size_t>> BurstDetectionEngine::findTimeBasedCandidates(
    const std::vector<PhotoInfo> &photos) {
  std::vector<std::vector<size_t>> candidates;
  std::vector<size_t> currentGroup;

  const int64_t maxTimeWindow = 10000; // 10秒窗口

  for (size_t i = 0; i < photos.size(); ++i) {
    if (currentGroup.empty()) {
      currentGroup.push_back(i);
    } else {
      int64_t timeGap =
          photos[i].timestamp - photos[currentGroup.back()].timestamp;
      if (timeGap <= maxTimeWindow) {
        currentGroup.push_back(i);
      } else {
        if (currentGroup.size() >= 2) {
          candidates.push_back(currentGroup);
        }
        currentGroup.clear();
        currentGroup.push_back(i);
      }
    }
  }

  if (currentGroup.size() >= 2) {
    candidates.push_back(currentGroup);
  }

  return candidates;
}

std::vector<std::vector<size_t>> BurstDetectionEngine::refineByTimingAnalysis(
    const std::vector<PhotoInfo> &photos,
    const std::vector<std::vector<size_t>> &candidates) {
  std::vector<std::vector<size_t>> refined;

  for (const auto &candidate : candidates) {
    if (candidate.size() < 2) {
      continue;
    }

    // 分析时间间隔
    std::vector<int64_t> intervals;
    for (size_t i = 1; i < candidate.size(); ++i) {
      int64_t gap =
          photos[candidate[i]].timestamp - photos[candidate[i - 1]].timestamp;
      intervals.push_back(gap);
    }

    // 检查间隔是否稳定且在合理范围内
    bool isValidBurst = true;
    for (int64_t interval : intervals) {
      if (!isTimeGapValid(interval)) {
        isValidBurst = false;
        break;
      }
    }

    if (isValidBurst && areTimingIntervalsStable(intervals)) {
      refined.push_back(candidate);
    } else {
      // 尝试分割成更小的有效分组
      std::vector<size_t> subGroup;
      for (size_t i = 0; i < candidate.size(); ++i) {
        if (subGroup.empty()) {
          subGroup.push_back(candidate[i]);
        } else {
          int64_t gap = photos[candidate[i]].timestamp -
                        photos[subGroup.back()].timestamp;
          if (isTimeGapValid(gap)) {
            subGroup.push_back(candidate[i]);
          } else {
            if (subGroup.size() >= 2) {
              refined.push_back(subGroup);
            }
            subGroup.clear();
            subGroup.push_back(candidate[i]);
          }
        }
      }
      if (subGroup.size() >= 2) {
        refined.push_back(subGroup);
      }
    }
  }

  return refined;
}

std::vector<std::vector<size_t>> BurstDetectionEngine::validateByExifInfo(
    const std::vector<PhotoInfo> &photos,
    const std::vector<std::vector<size_t>> &candidates) {
  std::vector<std::vector<size_t>> validated;

  for (const auto &candidate : candidates) {
    if (candidate.size() < 2) {
      continue;
    }

    // 检查EXIF信息一致性
    bool isConsistent = true;
    for (size_t i = 1; i < candidate.size(); ++i) {
      if (!areExifInfoSimilar(photos[candidate[0]], photos[candidate[i]])) {
        isConsistent = false;
        break;
      }
    }

    if (isConsistent) {
      validated.push_back(candidate);
    }
  }

  return validated;
}

std::vector<std::vector<size_t>> BurstDetectionEngine::checkFileNamePatterns(
    const std::vector<PhotoInfo> &photos,
    const std::vector<std::vector<size_t>> &candidates) {
  std::vector<std::vector<size_t>> finalCandidates;

  for (const auto &candidate : candidates) {
    if (candidate.size() < 2) {
      continue;
    }

    // 检查文件名序列模式
    bool hasSequentialNames = true;
    for (size_t i = 1; i < candidate.size(); ++i) {
      if (!areFileNamesSequential(photos[candidate[i - 1]].fileName,
                                  photos[candidate[i]].fileName)) {
        hasSequentialNames = false;
        break;
      }
    }

    // 如果文件名不连续，但其他条件满足，仍然可以认为是连拍
    // 这里可以根据需要调整策略
    finalCandidates.push_back(candidate);
  }

  return finalCandidates;
}

bool BurstDetectionEngine::arePhotosCompatible(const PhotoInfo &photo1,
                                               const PhotoInfo &photo2) const {
  // 检查基本兼容性
  if (photo1.width != photo2.width || photo1.height != photo2.height) {
    return false;
  }

  if (!photo1.cameraModel.empty() && !photo2.cameraModel.empty() &&
      photo1.cameraModel != photo2.cameraModel) {
    return false;
  }

  // GPS距离检查
  if (config_.enableGpsAnalysis && photo1.hasGpsInfo() && photo2.hasGpsInfo()) {
    double distance = calculateGpsDistance(photo1, photo2);
    if (distance > 100.0) { // 100米阈值
      return false;
    }
  }

  return true;
}

bool BurstDetectionEngine::isTimeGapValid(int64_t gap) const {
  return gap > 0 && gap <= config_.maxTimeGapMs;
}

bool BurstDetectionEngine::areTimingIntervalsStable(
    const std::vector<int64_t> &intervals) const {
  if (intervals.size() < 2) {
    return true;
  }

  // 计算平均间隔和标准差
  double sum = 0.0;
  for (int64_t interval : intervals) {
    sum += interval;
  }
  double mean = sum / intervals.size();

  double variance = 0.0;
  for (int64_t interval : intervals) {
    variance += (interval - mean) * (interval - mean);
  }
  double stddev = std::sqrt(variance / intervals.size());

  // 如果标准差超过均值的50%，认为不稳定
  return stddev <= mean * 0.5;
}

bool BurstDetectionEngine::areExifInfoSimilar(const PhotoInfo &photo1,
                                              const PhotoInfo &photo2) const {
  // 检查图片尺寸
  if (photo1.width != photo2.width || photo1.height != photo2.height) {
    return false;
  }

  // 检查相机型号
  if (!photo1.cameraModel.empty() && !photo2.cameraModel.empty() &&
      photo1.cameraModel != photo2.cameraModel) {
    return false;
  }

  return true;
}

bool BurstDetectionEngine::areFileNamesSequential(
    const std::string &name1, const std::string &name2) const {
  // 简单的数字序列检查
  std::regex numberRegex(R"((\d+))");
  std::smatch match1, match2;

  if (std::regex_search(name1, match1, numberRegex) &&
      std::regex_search(name2, match2, numberRegex)) {
    try {
      int num1 = std::stoi(match1[1].str());
      int num2 = std::stoi(match2[1].str());
      return num2 == num1 + 1;
    } catch (...) {
      return false;
    }
  }

  return false;
}

double
BurstDetectionEngine::calculateGpsDistance(const PhotoInfo &photo1,
                                           const PhotoInfo &photo2) const {
  if (!photo1.hasGpsInfo() || !photo2.hasGpsInfo()) {
    return 0.0;
  }

  // 使用 Haversine 公式计算距离
  const double R = 6371000; // 地球半径（米）
  double lat1Rad = photo1.latitude * M_PI / 180.0;
  double lat2Rad = photo2.latitude * M_PI / 180.0;
  double deltaLatRad = (photo2.latitude - photo1.latitude) * M_PI / 180.0;
  double deltaLonRad = (photo2.longitude - photo1.longitude) * M_PI / 180.0;

  double a = std::sin(deltaLatRad / 2) * std::sin(deltaLatRad / 2) +
             std::cos(lat1Rad) * std::cos(lat2Rad) * std::sin(deltaLonRad / 2) *
                 std::sin(deltaLonRad / 2);
  double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

  return R * c;
}

BurstGroup BurstDetectionEngine::createBurstGroup(
    const std::vector<PhotoInfo> &photos,
    const std::vector<size_t> &photoIndices) {
  BurstGroup group;
  group.groupId = generateGroupId();
  group.photoCount = static_cast<int32_t>(photoIndices.size());

  for (size_t idx : photoIndices) {
    group.photoIds.push_back(photos[idx].id);
  }

  if (!photoIndices.empty()) {
    group.startTime = photos[photoIndices.front()].timestamp;
    group.endTime = photos[photoIndices.back()].timestamp;
    group.representativePhotoId = selectRepresentativePhoto(group.photoIds);
  }

  return group;
}

std::string BurstDetectionEngine::generateGroupId() const {
  // 生成基于时间戳的唯一ID
  auto now = std::chrono::high_resolution_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                       now.time_since_epoch())
                       .count();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  std::stringstream ss;
  ss << "burst_" << timestamp << "_" << dis(gen);
  return ss.str();
}

std::string BurstDetectionEngine::selectRepresentativePhoto(
    const std::vector<std::string> &photoIds) const {
  if (photoIds.empty()) {
    return "";
  }

  // 选择中间位置的照片作为代表
  size_t middleIndex = photoIds.size() / 2;
  return photoIds[middleIndex];
}

std::vector<PhotoInfo> BurstDetectionEngine::sortPhotosByTimestamp(
    const std::vector<PhotoInfo> &photos) const {
  auto sorted = photos;
  std::sort(sorted.begin(), sorted.end(),
            [](const PhotoInfo &a, const PhotoInfo &b) {
              return a.timestamp < b.timestamp;
            });
  return sorted;
}

void BurstDetectionEngine::filterSmallGroups(
    std::vector<BurstGroup> &groups) const {
  groups.erase(std::remove_if(groups.begin(), groups.end(),
                              [this](const BurstGroup &group) {
                                return group.type == GroupType::BURST &&
                                       group.photoCount < config_.minBurstSize;
                              }),
               groups.end());
}

void BurstDetectionEngine::calculateStatistics(
    BurstDetectionResult &result) const {
  result.totalPhotos = 0;
  result.burstGroupCount = 0;
  result.singlePhotoCount = 0;

  for (const auto &group : result.groups) {
    result.totalPhotos += group.photoCount;
    if (group.type == GroupType::BURST) {
      result.burstGroupCount++;
    } else {
      result.singlePhotoCount++;
    }
  }
}

int64_t BurstDetectionEngine::getCurrentTimeMs() const {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

} // namespace facebook::react
