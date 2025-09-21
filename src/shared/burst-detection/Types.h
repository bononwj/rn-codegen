#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::react {

// 分组类型枚举
enum class GroupType {
  BURST,   // 连拍分组
  SIMILAR, // 相似照片分组
  SINGLE   // 单张照片
};

// 照片信息结构
struct PhotoInfo {
  std::string id;           // 唯一标识
  std::string filePath;     // 文件路径
  int64_t timestamp;        // 拍摄时间戳（毫秒）
  int64_t fileCreationTime; // 文件创建时间
  std::string fileName;     // 文件名
  int32_t width;            // 图片宽度
  int32_t height;           // 图片高度
  std::string cameraModel;  // 相机型号
  double latitude;          // GPS纬度 (可选, 默认为NaN)
  double longitude;         // GPS经度 (可选, 默认为NaN)
  std::string hashValue;    // 文件hash值 (可选)

  // 构造函数
  PhotoInfo()
      : timestamp(0), fileCreationTime(0), width(0), height(0),
        latitude(std::numeric_limits<double>::quiet_NaN()),
        longitude(std::numeric_limits<double>::quiet_NaN()) {}

  // 判断是否有GPS信息
  bool hasGpsInfo() const {
    return !std::isnan(latitude) && !std::isnan(longitude);
  }
};

// 连拍分组结构
struct BurstGroup {
  std::string groupId;               // 分组ID
  std::vector<std::string> photoIds; // 照片ID列表
  int64_t startTime;                 // 连拍开始时间
  int64_t endTime;                   // 连拍结束时间
  int32_t photoCount;                // 照片数量
  std::string representativePhotoId; // 代表照片ID (通常是第一张或中间张)
  GroupType type;                    // 分组类型

  // 构造函数
  BurstGroup()
      : startTime(0), endTime(0), photoCount(0), type(GroupType::SINGLE) {}

  // 获取类型字符串
  std::string getTypeString() const {
    switch (type) {
    case GroupType::BURST:
      return "BURST";
    case GroupType::SIMILAR:
      return "SIMILAR";
    case GroupType::SINGLE:
      return "SINGLE";
    default:
      return "SINGLE";
    }
  }
};

// 分组结果
struct BurstDetectionResult {
  std::vector<BurstGroup> groups; // 所有分组
  int32_t totalPhotos;            // 总照片数量
  int32_t burstGroupCount;        // 连拍分组数量
  int32_t singlePhotoCount;       // 单张照片数量
  int64_t processingTimeMs;       // 处理耗时

  // 构造函数
  BurstDetectionResult()
      : totalPhotos(0), burstGroupCount(0), singlePhotoCount(0),
        processingTimeMs(0) {}
};

// 连拍检测配置
struct BurstDetectionConfig {
  int64_t maxTimeGapMs;       // 最大时间间隔，默认2000ms
  int32_t minBurstSize;       // 最小连拍数量，默认3张
  bool enableGpsAnalysis;     // 是否启用GPS分析，默认false
  bool enableSimilarityCheck; // 是否启用相似度检查，默认false

  // 默认构造函数
  BurstDetectionConfig()
      : maxTimeGapMs(2000), minBurstSize(3), enableGpsAnalysis(false),
        enableSimilarityCheck(false) {}
};

// 增量更新结果
struct IncrementalUpdateResult {
  std::vector<BurstGroup> newGroups;         // 新创建的分组
  std::vector<std::string> modifiedGroupIds; // 被修改的分组ID
  std::vector<std::string> deletedGroupIds;  // 被删除的分组ID
  int64_t processingTimeMs;                  // 处理耗时

  // 构造函数
  IncrementalUpdateResult() : processingTimeMs(0) {}
};

// 性能统计信息
struct PerformanceStats {
  int32_t totalPhotos;          // 总照片数量
  int32_t totalGroups;          // 总分组数量
  double cacheHitRate;          // 缓存命中率
  int64_t lastProcessingTimeMs; // 最后处理耗时

  // 构造函数
  PerformanceStats()
      : totalPhotos(0), totalGroups(0), cacheHitRate(0.0),
        lastProcessingTimeMs(0) {}
};

// 时间窗口结构
struct TimeWindow {
  int64_t startTime;
  int64_t endTime;

  TimeWindow(int64_t start = 0, int64_t end = 0)
      : startTime(start), endTime(end) {}

  bool contains(int64_t timestamp) const {
    return timestamp >= startTime && timestamp <= endTime;
  }

  bool overlaps(const TimeWindow &other) const {
    return !(endTime < other.startTime || startTime > other.endTime);
  }
};

// 前向声明
class BurstDetectionEngine;
class PhotoGroupManager;
class IncrementalUpdater;
class BurstDetectionCache;
class TestDataGenerator;

} // namespace facebook::react
