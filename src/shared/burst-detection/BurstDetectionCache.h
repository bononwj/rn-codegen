#pragma once

#include "Types.h"
#include <chrono>
#include <list>
#include <unordered_map>

namespace facebook::react {

// LRU缓存模板类
template <typename Key, typename Value> class LRUCache {
public:
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}

  bool get(const Key &key, Value &value) {
    auto it = cache_.find(key);
    if (it == cache_.end()) {
      return false;
    }

    // 移动到最前面
    moveToFront(it->second);
    value = it->second->second;
    return true;
  }

  void put(const Key &key, const Value &value) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      // 更新现有值
      it->second->second = value;
      moveToFront(it->second);
    } else {
      // 插入新值
      if (cache_.size() >= capacity_) {
        evict();
      }
      items_.push_front({key, value});
      cache_[key] = items_.begin();
    }
  }

  void remove(const Key &key) {
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      items_.erase(it->second);
      cache_.erase(it);
    }
  }

  void clear() {
    cache_.clear();
    items_.clear();
  }

  size_t size() const { return cache_.size(); }

  bool empty() const { return cache_.empty(); }

private:
  using Item = std::pair<Key, Value>;
  using ItemIterator = typename std::list<Item>::iterator;

  size_t capacity_;
  std::list<Item> items_;
  std::unordered_map<Key, ItemIterator> cache_;

  void moveToFront(ItemIterator it) {
    items_.splice(items_.begin(), items_, it);
  }

  void evict() {
    if (!items_.empty()) {
      auto last = items_.back();
      cache_.erase(last.first);
      items_.pop_back();
    }
  }
};

// 缓存条目结构
struct CacheEntry {
  std::chrono::system_clock::time_point timestamp;
  size_t accessCount;

  CacheEntry() : timestamp(std::chrono::system_clock::now()), accessCount(1) {}

  void updateAccess() {
    timestamp = std::chrono::system_clock::now();
    accessCount++;
  }
};

class BurstDetectionCache {
public:
  explicit BurstDetectionCache(size_t maxGroupCacheSize = 1000,
                               size_t maxResultCacheSize = 100);
  ~BurstDetectionCache();

  // 分组缓存操作
  void cacheGroup(const std::string &groupId, const BurstGroup &group);
  bool getCachedGroup(const std::string &groupId, BurstGroup &group);
  void invalidateGroup(const std::string &groupId);
  void invalidateGroups(const std::vector<std::string> &groupIds);

  // 照片时间戳缓存
  void cachePhotoTimestamp(const std::string &photoId, int64_t timestamp);
  bool getCachedPhotoTimestamp(const std::string &photoId, int64_t &timestamp);
  void invalidatePhotoTimestamp(const std::string &photoId);

  // 分组结果缓存
  void cacheDetectionResult(const std::string &key,
                            const BurstDetectionResult &result);
  bool getCachedDetectionResult(const std::string &key,
                                BurstDetectionResult &result);
  void invalidateDetectionResults();

  // 照片分组关联缓存
  void cachePhotoGroups(const std::vector<std::string> &photoIds);
  void invalidatePhotoGroups(const std::vector<std::string> &photoIds);

  // 统计和管理
  void clearAll();
  double getCacheHitRate() const;
  size_t getTotalCacheSize() const;
  void evictExpiredEntries();
  void setExpirationTime(std::chrono::seconds expiration);

  // 性能统计
  struct CacheStats {
    size_t totalRequests;
    size_t totalHits;
    size_t groupCacheSize;
    size_t timestampCacheSize;
    size_t resultCacheSize;
    double hitRate;
  };

  CacheStats getStats() const;
  void resetStats();

private:
  // 缓存存储
  LRUCache<std::string, BurstGroup> groupCache_;
  std::unordered_map<std::string, CacheEntry> groupCacheEntries_;

  std::unordered_map<std::string, int64_t> photoTimestampCache_;
  std::unordered_map<std::string, CacheEntry> timestampCacheEntries_;

  LRUCache<std::string, BurstDetectionResult> resultCache_;
  std::unordered_map<std::string, CacheEntry> resultCacheEntries_;

  // 统计信息
  mutable size_t totalRequests_;
  mutable size_t totalHits_;
  mutable size_t groupCacheHits_;
  mutable size_t timestampCacheHits_;
  mutable size_t resultCacheHits_;

  // 配置
  std::chrono::seconds expirationTime_;

  // 内部方法
  bool isExpired(const CacheEntry &entry) const;
  void updateCacheEntry(std::unordered_map<std::string, CacheEntry> &entries,
                        const std::string &key);
  void
  cleanupExpiredEntries(std::unordered_map<std::string, CacheEntry> &entries);
  std::string generateResultCacheKey(const std::vector<PhotoInfo> &photos,
                                     const BurstDetectionConfig &config) const;
};

} // namespace facebook::react
