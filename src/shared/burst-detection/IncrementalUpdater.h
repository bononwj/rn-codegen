#pragma once

#include "BurstDetectionCache.h"
#include "BurstDetectionEngine.h"
#include "PhotoGroupManager.h"
#include "Types.h"
#include <memory>
#include <set>


namespace facebook::react {

class IncrementalUpdater {
public:
  IncrementalUpdater(std::shared_ptr<BurstDetectionEngine> engine,
                     std::shared_ptr<PhotoGroupManager> groupManager,
                     std::shared_ptr<BurstDetectionCache> cache);
  ~IncrementalUpdater();

  // 增量添加照片
  IncrementalUpdateResult addPhotos(const std::vector<PhotoInfo> &newPhotos);

  // 移除照片
  IncrementalUpdateResult
  removePhotos(const std::vector<std::string> &photoIds);

  // 更新照片信息
  IncrementalUpdateResult
  updatePhotos(const std::vector<PhotoInfo> &updatedPhotos);

  // 批量操作
  IncrementalUpdateResult
  batchUpdate(const std::vector<PhotoInfo> &addedPhotos,
              const std::vector<std::string> &removedPhotoIds,
              const std::vector<PhotoInfo> &updatedPhotos);

  // 配置管理
  void updateConfig(const BurstDetectionConfig &config);
  const BurstDetectionConfig &getConfig() const;

  // 性能优化控制
  void setOptimizationLevel(int level); // 0=基础, 1=标准, 2=激进
  void enableBatchProcessing(bool enable);
  void setMinBatchSize(size_t minSize);

private:
  // 核心组件引用
  std::shared_ptr<BurstDetectionEngine> engine_;
  std::shared_ptr<PhotoGroupManager> groupManager_;
  std::shared_ptr<BurstDetectionCache> cache_;

  // 配置和状态
  BurstDetectionConfig config_;
  int optimizationLevel_;
  bool batchProcessingEnabled_;
  size_t minBatchSize_;

  // 内部状态维护
  std::map<int64_t, std::vector<PhotoInfo>> timeIndex_;   // 时间索引
  std::unordered_map<std::string, PhotoInfo> photoIndex_; // 照片索引

  // 核心增量算法
  IncrementalUpdateResult
  processAddition(const std::vector<PhotoInfo> &newPhotos);
  IncrementalUpdateResult
  processRemoval(const std::vector<std::string> &photoIds);
  IncrementalUpdateResult
  processUpdate(const std::vector<PhotoInfo> &updatedPhotos);

  // 影响分析
  TimeWindow findAffectedTimeWindow(const PhotoInfo &photo) const;
  TimeWindow findAffectedTimeWindow(const std::vector<PhotoInfo> &photos) const;
  std::set<std::string> findAffectedGroups(const TimeWindow &window) const;
  std::set<std::string>
  findGroupsByPhotoIds(const std::vector<std::string> &photoIds) const;

  // 局部重新分组
  std::vector<BurstGroup> reprocessTimeWindow(const TimeWindow &window);
  std::vector<PhotoInfo> getPhotosInTimeWindow(const TimeWindow &window) const;
  void mergeAdjacentTimeWindows(std::vector<TimeWindow> &windows) const;

  // 分组变更分析
  struct GroupChangeAnalysis {
    std::vector<BurstGroup> newGroups;
    std::vector<std::string> modifiedGroupIds;
    std::vector<std::string> deletedGroupIds;
    std::set<std::string> affectedPhotoIds;
  };

  GroupChangeAnalysis
  analyzeGroupChanges(const std::set<std::string> &affectedGroupIds,
                      const std::vector<BurstGroup> &newGroups) const;

  // 优化策略
  bool shouldUseFullReprocessing(const std::vector<PhotoInfo> &photos) const;
  bool shouldMergeTimeWindows(const TimeWindow &window1,
                              const TimeWindow &window2) const;
  void optimizeBatchSize(std::vector<PhotoInfo> &photos) const;

  // 索引维护
  void updateTimeIndex(const std::vector<PhotoInfo> &photos);
  void removeFromTimeIndex(const std::vector<std::string> &photoIds);
  void updatePhotoIndex(const std::vector<PhotoInfo> &photos);
  void removeFromPhotoIndex(const std::vector<std::string> &photoIds);

  // 缓存管理
  void invalidateAffectedCache(const std::set<std::string> &affectedGroupIds,
                               const std::vector<std::string> &photoIds);
  void updateCacheAfterChanges(const GroupChangeAnalysis &analysis);

  // 验证和一致性检查
  bool validateGroupConsistency(const BurstGroup &group) const;
  void ensureDataConsistency();

  // 性能监控
  struct PerformanceMetrics {
    size_t totalOperations;
    size_t fullReprocessingCount;
    size_t incrementalUpdateCount;
    int64_t totalProcessingTime;
    int64_t averageProcessingTime;
  };

  mutable PerformanceMetrics metrics_;
  void updateMetrics(int64_t processingTime, bool wasFullReprocessing);

  // 工具方法
  int64_t getCurrentTimeMs() const;
  std::string generateUpdateId() const;
  bool isPhotoInTimeWindow(const PhotoInfo &photo,
                           const TimeWindow &window) const;

  // 时间窗口工具
  static constexpr int64_t DEFAULT_TIME_BUFFER_MS = 5000; // 5秒缓冲
  TimeWindow expandTimeWindow(const TimeWindow &window,
                              int64_t bufferMs = DEFAULT_TIME_BUFFER_MS) const;
  bool timeWindowsOverlap(const TimeWindow &w1, const TimeWindow &w2) const;
  TimeWindow mergeTimeWindows(const TimeWindow &w1, const TimeWindow &w2) const;
};

} // namespace facebook::react
