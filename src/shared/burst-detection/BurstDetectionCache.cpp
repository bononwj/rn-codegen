#include "BurstDetectionCache.h"
#include <algorithm>
#include <sstream>

namespace facebook::react {

BurstDetectionCache::BurstDetectionCache(size_t maxGroupCacheSize,
                                         size_t maxResultCacheSize)
    : groupCache_(maxGroupCacheSize), resultCache_(maxResultCacheSize),
      totalRequests_(0), totalHits_(0), groupCacheHits_(0),
      timestampCacheHits_(0), resultCacheHits_(0),
      expirationTime_(std::chrono::hours(1)) {}

BurstDetectionCache::~BurstDetectionCache() = default;

void BurstDetectionCache::cacheGroup(const std::string &groupId,
                                     const BurstGroup &group) {
  groupCache_.put(groupId, group);
  groupCacheEntries_[groupId] = CacheEntry();
}

bool BurstDetectionCache::getCachedGroup(const std::string &groupId,
                                         BurstGroup &group) {
  totalRequests_++;

  auto entryIt = groupCacheEntries_.find(groupId);
  if (entryIt != groupCacheEntries_.end() && isExpired(entryIt->second)) {
    // 过期条目
    invalidateGroup(groupId);
    return false;
  }

  if (groupCache_.get(groupId, group)) {
    totalHits_++;
    groupCacheHits_++;
    updateCacheEntry(groupCacheEntries_, groupId);
    return true;
  }

  return false;
}

void BurstDetectionCache::invalidateGroup(const std::string &groupId) {
  groupCache_.remove(groupId);
  groupCacheEntries_.erase(groupId);
}

void BurstDetectionCache::invalidateGroups(
    const std::vector<std::string> &groupIds) {
  for (const auto &groupId : groupIds) {
    invalidateGroup(groupId);
  }
}

void BurstDetectionCache::cachePhotoTimestamp(const std::string &photoId,
                                              int64_t timestamp) {
  photoTimestampCache_[photoId] = timestamp;
  timestampCacheEntries_[photoId] = CacheEntry();
}

bool BurstDetectionCache::getCachedPhotoTimestamp(const std::string &photoId,
                                                  int64_t &timestamp) {
  totalRequests_++;

  auto entryIt = timestampCacheEntries_.find(photoId);
  if (entryIt != timestampCacheEntries_.end() && isExpired(entryIt->second)) {
    // 过期条目
    invalidatePhotoTimestamp(photoId);
    return false;
  }

  auto it = photoTimestampCache_.find(photoId);
  if (it != photoTimestampCache_.end()) {
    timestamp = it->second;
    totalHits_++;
    timestampCacheHits_++;
    updateCacheEntry(timestampCacheEntries_, photoId);
    return true;
  }

  return false;
}

void BurstDetectionCache::invalidatePhotoTimestamp(const std::string &photoId) {
  photoTimestampCache_.erase(photoId);
  timestampCacheEntries_.erase(photoId);
}

void BurstDetectionCache::cacheDetectionResult(
    const std::string &key, const BurstDetectionResult &result) {
  resultCache_.put(key, result);
  resultCacheEntries_[key] = CacheEntry();
}

bool BurstDetectionCache::getCachedDetectionResult(
    const std::string &key, BurstDetectionResult &result) {
  totalRequests_++;

  auto entryIt = resultCacheEntries_.find(key);
  if (entryIt != resultCacheEntries_.end() && isExpired(entryIt->second)) {
    // 过期条目
    resultCache_.remove(key);
    resultCacheEntries_.erase(key);
    return false;
  }

  if (resultCache_.get(key, result)) {
    totalHits_++;
    resultCacheHits_++;
    updateCacheEntry(resultCacheEntries_, key);
    return true;
  }

  return false;
}

void BurstDetectionCache::invalidateDetectionResults() {
  resultCache_.clear();
  resultCacheEntries_.clear();
}

void BurstDetectionCache::cachePhotoGroups(
    const std::vector<std::string> &photoIds) {
  // 预加载相关照片的时间戳缓存
  for (const auto &photoId : photoIds) {
    // 这里可以添加预加载逻辑
  }
}

void BurstDetectionCache::invalidatePhotoGroups(
    const std::vector<std::string> &photoIds) {
  for (const auto &photoId : photoIds) {
    invalidatePhotoTimestamp(photoId);
  }

  // 同时清理可能相关的结果缓存
  invalidateDetectionResults();
}

void BurstDetectionCache::clearAll() {
  groupCache_.clear();
  groupCacheEntries_.clear();
  photoTimestampCache_.clear();
  timestampCacheEntries_.clear();
  resultCache_.clear();
  resultCacheEntries_.clear();
  resetStats();
}

double BurstDetectionCache::getCacheHitRate() const {
  if (totalRequests_ == 0) {
    return 0.0;
  }
  return static_cast<double>(totalHits_) / static_cast<double>(totalRequests_);
}

size_t BurstDetectionCache::getTotalCacheSize() const {
  return groupCache_.size() + photoTimestampCache_.size() + resultCache_.size();
}

void BurstDetectionCache::evictExpiredEntries() {
  cleanupExpiredEntries(groupCacheEntries_);
  cleanupExpiredEntries(timestampCacheEntries_);
  cleanupExpiredEntries(resultCacheEntries_);
}

void BurstDetectionCache::setExpirationTime(std::chrono::seconds expiration) {
  expirationTime_ = expiration;
}

BurstDetectionCache::CacheStats BurstDetectionCache::getStats() const {
  CacheStats stats;
  stats.totalRequests = totalRequests_;
  stats.totalHits = totalHits_;
  stats.groupCacheSize = groupCache_.size();
  stats.timestampCacheSize = photoTimestampCache_.size();
  stats.resultCacheSize = resultCache_.size();
  stats.hitRate = getCacheHitRate();
  return stats;
}

void BurstDetectionCache::resetStats() {
  totalRequests_ = 0;
  totalHits_ = 0;
  groupCacheHits_ = 0;
  timestampCacheHits_ = 0;
  resultCacheHits_ = 0;
}

bool BurstDetectionCache::isExpired(const CacheEntry &entry) const {
  auto now = std::chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp);
  return duration > expirationTime_;
}

void BurstDetectionCache::updateCacheEntry(
    std::unordered_map<std::string, CacheEntry> &entries,
    const std::string &key) {
  auto it = entries.find(key);
  if (it != entries.end()) {
    it->second.updateAccess();
  }
}

void BurstDetectionCache::cleanupExpiredEntries(
    std::unordered_map<std::string, CacheEntry> &entries) {
  auto it = entries.begin();
  while (it != entries.end()) {
    if (isExpired(it->second)) {
      // 同时清理对应的缓存数据
      if (&entries == &groupCacheEntries_) {
        groupCache_.remove(it->first);
      } else if (&entries == &timestampCacheEntries_) {
        photoTimestampCache_.erase(it->first);
      } else if (&entries == &resultCacheEntries_) {
        resultCache_.remove(it->first);
      }
      it = entries.erase(it);
    } else {
      ++it;
    }
  }
}

std::string BurstDetectionCache::generateResultCacheKey(
    const std::vector<PhotoInfo> &photos,
    const BurstDetectionConfig &config) const {
  std::stringstream ss;
  ss << "result_" << photos.size() << "_" << config.maxTimeGapMs << "_"
     << config.minBurstSize << "_" << config.enableGpsAnalysis << "_"
     << config.enableSimilarityCheck;

  // 添加照片ID的哈希值
  std::hash<std::string> hasher;
  size_t combinedHash = 0;
  for (const auto &photo : photos) {
    combinedHash ^= hasher(photo.id) + 0x9e3779b9 + (combinedHash << 6) +
                    (combinedHash >> 2);
  }
  ss << "_" << combinedHash;

  return ss.str();
}

} // namespace facebook::react