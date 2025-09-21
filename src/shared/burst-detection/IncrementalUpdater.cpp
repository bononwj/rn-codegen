#include "IncrementalUpdater.h"
#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>

namespace facebook::react {

IncrementalUpdater::IncrementalUpdater(
    std::shared_ptr<BurstDetectionEngine> engine,
    std::shared_ptr<PhotoGroupManager> groupManager,
    std::shared_ptr<BurstDetectionCache> cache)
    : engine_(engine), groupManager_(groupManager), cache_(cache),
      optimizationLevel_(1), batchProcessingEnabled_(true), minBatchSize_(10),
      metrics_{} {
  if (engine_) {
    config_ = engine_->getConfig();
  }
}

IncrementalUpdater::~IncrementalUpdater() = default;

IncrementalUpdateResult
IncrementalUpdater::addPhotos(const std::vector<PhotoInfo> &newPhotos) {
  return processAddition(newPhotos);
}

IncrementalUpdateResult
IncrementalUpdater::removePhotos(const std::vector<std::string> &photoIds) {
  return processRemoval(photoIds);
}

IncrementalUpdateResult
IncrementalUpdater::updatePhotos(const std::vector<PhotoInfo> &updatedPhotos) {
  return processUpdate(updatedPhotos);
}

IncrementalUpdateResult
IncrementalUpdater::batchUpdate(const std::vector<PhotoInfo> &addedPhotos,
                                const std::vector<std::string> &removedPhotoIds,
                                const std::vector<PhotoInfo> &updatedPhotos) {

  auto startTime = getCurrentTimeMs();
  IncrementalUpdateResult result;

  try {
    // 1. 先处理移除操作
    if (!removedPhotoIds.empty()) {
      auto removeResult = processRemoval(removedPhotoIds);
      result.deletedGroupIds = removeResult.deletedGroupIds;
      result.modifiedGroupIds.insert(result.modifiedGroupIds.end(),
                                     removeResult.modifiedGroupIds.begin(),
                                     removeResult.modifiedGroupIds.end());
    }

    // 2. 处理更新操作
    if (!updatedPhotos.empty()) {
      auto updateResult = processUpdate(updatedPhotos);
      result.modifiedGroupIds.insert(result.modifiedGroupIds.end(),
                                     updateResult.modifiedGroupIds.begin(),
                                     updateResult.modifiedGroupIds.end());
      result.newGroups.insert(result.newGroups.end(),
                              updateResult.newGroups.begin(),
                              updateResult.newGroups.end());
    }

    // 3. 最后处理添加操作
    if (!addedPhotos.empty()) {
      auto addResult = processAddition(addedPhotos);
      result.newGroups.insert(result.newGroups.end(),
                              addResult.newGroups.begin(),
                              addResult.newGroups.end());
      result.modifiedGroupIds.insert(result.modifiedGroupIds.end(),
                                     addResult.modifiedGroupIds.begin(),
                                     addResult.modifiedGroupIds.end());
    }

    // 4. 去重修改的分组ID
    std::sort(result.modifiedGroupIds.begin(), result.modifiedGroupIds.end());
    result.modifiedGroupIds.erase(std::unique(result.modifiedGroupIds.begin(),
                                              result.modifiedGroupIds.end()),
                                  result.modifiedGroupIds.end());

    result.processingTimeMs = getCurrentTimeMs() - startTime;
    updateMetrics(result.processingTimeMs, false);

  } catch (const std::exception &e) {
    result.processingTimeMs = getCurrentTimeMs() - startTime;
    // 记录错误但不抛出异常
  }

  return result;
}

IncrementalUpdateResult
IncrementalUpdater::processAddition(const std::vector<PhotoInfo> &newPhotos) {
  auto startTime = getCurrentTimeMs();
  IncrementalUpdateResult result;

  if (newPhotos.empty()) {
    return result;
  }

  // 检查是否应该使用全量重处理
  if (shouldUseFullReprocessing(newPhotos)) {
    // 获取所有现有照片并重新处理
    auto allGroups = groupManager_->getAllGroups();
    std::vector<PhotoInfo> allPhotos;

    // 从现有分组中收集所有照片信息
    for (const auto &group : allGroups) {
      for (const auto &photoId : group.photoIds) {
        auto it = photoIndex_.find(photoId);
        if (it != photoIndex_.end()) {
          allPhotos.push_back(it->second);
        }
      }
    }

    // 添加新照片
    allPhotos.insert(allPhotos.end(), newPhotos.begin(), newPhotos.end());

    // 全量重新检测
    auto detectionResult = engine_->detectBursts(allPhotos);

    // 清除现有分组并添加新分组
    groupManager_->clearAllGroups();
    for (const auto &group : detectionResult.groups) {
      groupManager_->addGroup(group);
      result.newGroups.push_back(group);
    }

    updateMetrics(getCurrentTimeMs() - startTime, true);
    result.processingTimeMs = getCurrentTimeMs() - startTime;
    return result;
  }

  // 增量处理
  // 1. 找到受影响的时间窗口
  auto affectedWindow = findAffectedTimeWindow(newPhotos);
  affectedWindow = expandTimeWindow(affectedWindow);

  // 2. 找到受影响的分组
  auto affectedGroupIds = findAffectedGroups(affectedWindow);

  // 3. 收集时间窗口内的所有照片（包括新照片）
  auto windowPhotos = getPhotosInTimeWindow(affectedWindow);
  windowPhotos.insert(windowPhotos.end(), newPhotos.begin(), newPhotos.end());

  // 4. 重新处理时间窗口
  auto newGroups = reprocessTimeWindow(affectedWindow);

  // 5. 分析分组变更
  auto analysis = analyzeGroupChanges(affectedGroupIds, newGroups);

  // 6. 应用变更到分组管理器
  for (const auto &groupId : analysis.deletedGroupIds) {
    groupManager_->removeGroup(groupId);
  }

  for (const auto &group : analysis.newGroups) {
    groupManager_->addGroup(group);
  }

  // 7. 更新索引
  updateTimeIndex(newPhotos);
  updatePhotoIndex(newPhotos);

  // 8. 更新缓存
  updateCacheAfterChanges(analysis);

  result.newGroups = analysis.newGroups;
  result.modifiedGroupIds = analysis.modifiedGroupIds;
  result.deletedGroupIds = analysis.deletedGroupIds;
  result.processingTimeMs = getCurrentTimeMs() - startTime;

  updateMetrics(result.processingTimeMs, false);
  return result;
}

IncrementalUpdateResult
IncrementalUpdater::processRemoval(const std::vector<std::string> &photoIds) {
  auto startTime = getCurrentTimeMs();
  IncrementalUpdateResult result;

  if (photoIds.empty()) {
    return result;
  }

  // 1. 找到包含这些照片的分组
  auto affectedGroupIds = findGroupsByPhotoIds(photoIds);

  // 2. 从分组中移除照片
  for (const auto &groupId : affectedGroupIds) {
    for (const auto &photoId : photoIds) {
      groupManager_->removePhotoFromGroup(photoId, groupId);
    }
  }

  // 3. 清理空分组
  groupManager_->removeEmptyGroups();

  // 4. 检查剩余照片是否还能形成有效连拍
  for (const auto &groupId : affectedGroupIds) {
    auto group = groupManager_->getGroup(groupId);
    if (group && group->type == GroupType::BURST) {
      if (group->photoCount < config_.minBurstSize) {
        // 将连拍分组转换为单张分组
        auto photoIds = group->photoIds;
        groupManager_->removeGroup(groupId);

        for (const auto &photoId : photoIds) {
          BurstGroup singleGroup;
          singleGroup.groupId = generateUpdateId();
          singleGroup.photoIds = {photoId};
          singleGroup.photoCount = 1;
          singleGroup.type = GroupType::SINGLE;
          singleGroup.representativePhotoId = photoId;

          groupManager_->addGroup(singleGroup);
          result.newGroups.push_back(singleGroup);
        }

        result.deletedGroupIds.push_back(groupId);
      } else {
        result.modifiedGroupIds.push_back(groupId);
      }
    }
  }

  // 5. 更新索引
  removeFromTimeIndex(photoIds);
  removeFromPhotoIndex(photoIds);

  // 6. 失效相关缓存
  std::vector<std::string> affectedGroupIdsList(affectedGroupIds.begin(),
                                                affectedGroupIds.end());
  invalidateAffectedCache(affectedGroupIds, photoIds);

  result.processingTimeMs = getCurrentTimeMs() - startTime;
  updateMetrics(result.processingTimeMs, false);
  return result;
}

IncrementalUpdateResult
IncrementalUpdater::processUpdate(const std::vector<PhotoInfo> &updatedPhotos) {
  auto startTime = getCurrentTimeMs();
  IncrementalUpdateResult result;

  if (updatedPhotos.empty()) {
    return result;
  }

  // 更新操作等同于先删除再添加
  std::vector<std::string> photoIds;
  for (const auto &photo : updatedPhotos) {
    photoIds.push_back(photo.id);
  }

  auto removeResult = processRemoval(photoIds);
  auto addResult = processAddition(updatedPhotos);

  // 合并结果
  result.newGroups = addResult.newGroups;
  result.modifiedGroupIds = removeResult.modifiedGroupIds;
  result.modifiedGroupIds.insert(result.modifiedGroupIds.end(),
                                 addResult.modifiedGroupIds.begin(),
                                 addResult.modifiedGroupIds.end());
  result.deletedGroupIds = removeResult.deletedGroupIds;
  result.deletedGroupIds.insert(result.deletedGroupIds.end(),
                                addResult.deletedGroupIds.begin(),
                                addResult.deletedGroupIds.end());

  result.processingTimeMs = getCurrentTimeMs() - startTime;
  updateMetrics(result.processingTimeMs, false);
  return result;
}

TimeWindow
IncrementalUpdater::findAffectedTimeWindow(const PhotoInfo &photo) const {
  int64_t buffer = DEFAULT_TIME_BUFFER_MS;
  return TimeWindow(photo.timestamp - buffer, photo.timestamp + buffer);
}

TimeWindow IncrementalUpdater::findAffectedTimeWindow(
    const std::vector<PhotoInfo> &photos) const {
  if (photos.empty()) {
    return TimeWindow();
  }

  int64_t minTime = photos[0].timestamp;
  int64_t maxTime = photos[0].timestamp;

  for (const auto &photo : photos) {
    minTime = std::min(minTime, photo.timestamp);
    maxTime = std::max(maxTime, photo.timestamp);
  }

  int64_t buffer = DEFAULT_TIME_BUFFER_MS;
  return TimeWindow(minTime - buffer, maxTime + buffer);
}

std::set<std::string>
IncrementalUpdater::findAffectedGroups(const TimeWindow &window) const {
  std::set<std::string> affectedGroups;

  auto allGroups = groupManager_->getAllGroups();
  for (const auto &group : allGroups) {
    if (window.overlaps(TimeWindow(group.startTime, group.endTime))) {
      affectedGroups.insert(group.groupId);
    }
  }

  return affectedGroups;
}

std::set<std::string> IncrementalUpdater::findGroupsByPhotoIds(
    const std::vector<std::string> &photoIds) const {
  std::set<std::string> groupIds;

  for (const auto &photoId : photoIds) {
    auto groupId = groupManager_->getPhotoGroup(photoId);
    if (!groupId.empty()) {
      groupIds.insert(groupId);
    }
  }

  return groupIds;
}

std::vector<PhotoInfo>
IncrementalUpdater::getPhotosInTimeWindow(const TimeWindow &window) const {
  std::vector<PhotoInfo> photos;

  for (const auto &[photoId, photoInfo] : photoIndex_) {
    if (window.contains(photoInfo.timestamp)) {
      photos.push_back(photoInfo);
    }
  }

  return photos;
}

std::vector<BurstGroup>
IncrementalUpdater::reprocessTimeWindow(const TimeWindow &window) {
  auto photos = getPhotosInTimeWindow(window);
  if (photos.empty()) {
    return {};
  }

  auto result = engine_->detectBursts(photos);
  return result.groups;
}

IncrementalUpdater::GroupChangeAnalysis IncrementalUpdater::analyzeGroupChanges(
    const std::set<std::string> &affectedGroupIds,
    const std::vector<BurstGroup> &newGroups) const {

  GroupChangeAnalysis analysis;

  // 所有受影响的旧分组都被删除
  analysis.deletedGroupIds.assign(affectedGroupIds.begin(),
                                  affectedGroupIds.end());

  // 所有新分组都是新创建的
  analysis.newGroups = newGroups;

  return analysis;
}

bool IncrementalUpdater::shouldUseFullReprocessing(
    const std::vector<PhotoInfo> &photos) const {

  // 如果新照片数量很大，使用全量重处理
  if (photos.size() > 1000) {
    return true;
  }

  // 如果优化级别设置为基础，优先使用全量重处理
  if (optimizationLevel_ == 0) {
    return photos.size() > 50;
  }

  return false;
}

void IncrementalUpdater::updateTimeIndex(const std::vector<PhotoInfo> &photos) {
  for (const auto &photo : photos) {
    timeIndex_[photo.timestamp].push_back(photo);
  }
}

void IncrementalUpdater::removeFromTimeIndex(
    const std::vector<std::string> &photoIds) {
  for (const auto &photoId : photoIds) {
    auto it = photoIndex_.find(photoId);
    if (it != photoIndex_.end()) {
      int64_t timestamp = it->second.timestamp;
      auto timeIt = timeIndex_.find(timestamp);
      if (timeIt != timeIndex_.end()) {
        auto &photos = timeIt->second;
        photos.erase(std::remove_if(photos.begin(), photos.end(),
                                    [&photoId](const PhotoInfo &p) {
                                      return p.id == photoId;
                                    }),
                     photos.end());
        if (photos.empty()) {
          timeIndex_.erase(timeIt);
        }
      }
    }
  }
}

void IncrementalUpdater::updatePhotoIndex(
    const std::vector<PhotoInfo> &photos) {
  for (const auto &photo : photos) {
    photoIndex_[photo.id] = photo;
  }
}

void IncrementalUpdater::removeFromPhotoIndex(
    const std::vector<std::string> &photoIds) {
  for (const auto &photoId : photoIds) {
    photoIndex_.erase(photoId);
  }
}

void IncrementalUpdater::invalidateAffectedCache(
    const std::set<std::string> &affectedGroupIds,
    const std::vector<std::string> &photoIds) {

  std::vector<std::string> groupIdsList(affectedGroupIds.begin(),
                                        affectedGroupIds.end());
  cache_->invalidateGroups(groupIdsList);
  cache_->invalidatePhotoGroups(photoIds);

  for (const auto &photoId : photoIds) {
    cache_->invalidatePhotoTimestamp(photoId);
  }
}

void IncrementalUpdater::updateCacheAfterChanges(
    const GroupChangeAnalysis &analysis) {

  // 缓存新分组
  for (const auto &group : analysis.newGroups) {
    cache_->cacheGroup(group.groupId, group);
  }
}

void IncrementalUpdater::updateConfig(const BurstDetectionConfig &config) {
  config_ = config;
  if (engine_) {
    engine_->updateConfig(config);
  }
}

const BurstDetectionConfig &IncrementalUpdater::getConfig() const {
  return config_;
}

void IncrementalUpdater::setOptimizationLevel(int level) {
  optimizationLevel_ = std::max(0, std::min(2, level));
}

void IncrementalUpdater::enableBatchProcessing(bool enable) {
  batchProcessingEnabled_ = enable;
}

void IncrementalUpdater::setMinBatchSize(size_t minSize) {
  minBatchSize_ = minSize;
}

TimeWindow IncrementalUpdater::expandTimeWindow(const TimeWindow &window,
                                                int64_t bufferMs) const {
  return TimeWindow(window.startTime - bufferMs, window.endTime + bufferMs);
}

void IncrementalUpdater::updateMetrics(int64_t processingTime,
                                       bool wasFullReprocessing) {
  metrics_.totalOperations++;
  metrics_.totalProcessingTime += processingTime;
  metrics_.averageProcessingTime =
      metrics_.totalProcessingTime / metrics_.totalOperations;

  if (wasFullReprocessing) {
    metrics_.fullReprocessingCount++;
  } else {
    metrics_.incrementalUpdateCount++;
  }
}

int64_t IncrementalUpdater::getCurrentTimeMs() const {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

std::string IncrementalUpdater::generateUpdateId() const {
  auto now = std::chrono::high_resolution_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                       now.time_since_epoch())
                       .count();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  std::stringstream ss;
  ss << "update_" << timestamp << "_" << dis(gen);
  return ss.str();
}

} // namespace facebook::react
