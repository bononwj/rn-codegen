#include "NativePhotoBurstDetection.h"
#include <chrono>
#include <future>
#include <mutex>
#include <thread>

namespace facebook::react {

NativePhotoBurstDetection::NativePhotoBurstDetection(
    std::shared_ptr<CallInvoker> jsInvoker)
    : NativePhotoBurstDetectionCxxSpec(std::move(jsInvoker)),
      isInitialized_(false) {
  initializeComponents();
}

NativePhotoBurstDetection::~NativePhotoBurstDetection() { cleanupComponents(); }

void NativePhotoBurstDetection::initializeComponents() {
  std::lock_guard<std::mutex> lock(operationMutex_);

  // 初始化核心组件
  engine_ = std::make_shared<BurstDetectionEngine>(currentConfig_);
  groupManager_ = std::make_shared<PhotoGroupManager>();
  cache_ = std::make_shared<BurstDetectionCache>();
  incrementalUpdater_ =
      std::make_shared<IncrementalUpdater>(engine_, groupManager_, cache_);
  testDataGenerator_ = std::make_shared<TestDataGenerator>();

  isInitialized_ = true;
}

void NativePhotoBurstDetection::cleanupComponents() {
  std::lock_guard<std::mutex> lock(operationMutex_);

  testDataGenerator_.reset();
  incrementalUpdater_.reset();
  cache_.reset();
  groupManager_.reset();
  engine_.reset();

  isInitialized_ = false;
}

// 核心检测接口实现
jsi::Value NativePhotoBurstDetection::detectBursts(jsi::Runtime &rt,
                                                   jsi::Array photos,
                                                   jsi::Object config) {
  return executeAsync<BurstDetectionResult>(
      rt,
      [this, &rt, &photos, &config]() -> BurstDetectionResult {
        auto photoList = parsePhotoArray(rt, photos);

        if (!config.isUndefined()) {
          auto configData = parseConfig(rt, config);
          engine_->updateConfig(configData);
        }

        auto result = engine_->detectBursts(photoList);

        // 将结果存储到分组管理器
        groupManager_->clearAllGroups();
        for (const auto &group : result.groups) {
          groupManager_->addGroup(group);
        }

        return result;
      },
      [this](jsi::Runtime &rt, const BurstDetectionResult &result) {
        return createDetectionResultObject(rt, result);
      });
}

// 增量更新接口实现
jsi::Value NativePhotoBurstDetection::addPhotos(jsi::Runtime &rt,
                                                jsi::Array photos) {
  return executeAsync<IncrementalUpdateResult>(
      rt,
      [this, &rt, &photos]() -> IncrementalUpdateResult {
        auto photoList = parsePhotoArray(rt, photos);
        return incrementalUpdater_->addPhotos(photoList);
      },
      [this](jsi::Runtime &rt, const IncrementalUpdateResult &result) {
        return createIncrementalUpdateResultObject(rt, result);
      });
}

jsi::Value NativePhotoBurstDetection::removePhotos(jsi::Runtime &rt,
                                                   jsi::Array photoIds) {
  return executeAsync<IncrementalUpdateResult>(
      rt,
      [this, &rt, &photoIds]() -> IncrementalUpdateResult {
        auto idList = parseStringArray(rt, photoIds);
        return incrementalUpdater_->removePhotos(idList);
      },
      [this](jsi::Runtime &rt, const IncrementalUpdateResult &result) {
        return createIncrementalUpdateResultObject(rt, result);
      });
}

jsi::Value NativePhotoBurstDetection::updatePhotos(jsi::Runtime &rt,
                                                   jsi::Array photos) {
  return executeAsync<IncrementalUpdateResult>(
      rt,
      [this, &rt, &photos]() -> IncrementalUpdateResult {
        auto photoList = parsePhotoArray(rt, photos);
        return incrementalUpdater_->updatePhotos(photoList);
      },
      [this](jsi::Runtime &rt, const IncrementalUpdateResult &result) {
        return createIncrementalUpdateResultObject(rt, result);
      });
}

jsi::Value NativePhotoBurstDetection::batchUpdate(jsi::Runtime &rt,
                                                  jsi::Array addedPhotos,
                                                  jsi::Array removedPhotoIds,
                                                  jsi::Array updatedPhotos) {
  return executeAsync<IncrementalUpdateResult>(
      rt,
      [this, &rt, &addedPhotos, &removedPhotoIds,
       &updatedPhotos]() -> IncrementalUpdateResult {
        auto addedList = parsePhotoArray(rt, addedPhotos);
        auto removedList = parseStringArray(rt, removedPhotoIds);
        auto updatedList = parsePhotoArray(rt, updatedPhotos);
        return incrementalUpdater_->batchUpdate(addedList, removedList,
                                                updatedList);
      },
      [this](jsi::Runtime &rt, const IncrementalUpdateResult &result) {
        return createIncrementalUpdateResultObject(rt, result);
      });
}

// 查询接口实现
jsi::Value NativePhotoBurstDetection::getGroupDetails(jsi::Runtime &rt,
                                                      std::string groupId) {
  return executeAsync<BurstGroup *>(
      rt,
      [this, groupId]() -> BurstGroup * {
        return groupManager_->getGroup(groupId);
      },
      [this](jsi::Runtime &rt, BurstGroup *const &group) -> jsi::Value {
        if (group) {
          return createBurstGroupObject(rt, *group);
        } else {
          return jsi::Value::null();
        }
      });
}

jsi::Value NativePhotoBurstDetection::getAllGroups(jsi::Runtime &rt) {
  return executeAsync<std::vector<BurstGroup>>(
      rt,
      [this]() -> std::vector<BurstGroup> {
        return groupManager_->getAllGroups();
      },
      [this](jsi::Runtime &rt, const std::vector<BurstGroup> &groups) {
        return createBurstGroupArray(rt, groups);
      });
}

jsi::Value NativePhotoBurstDetection::getPhotoGroup(jsi::Runtime &rt,
                                                    std::string photoId) {
  return executeAsync<std::string>(
      rt,
      [this, photoId]() -> std::string {
        return groupManager_->getPhotoGroup(photoId);
      },
      [](jsi::Runtime &rt, const std::string &groupId) -> jsi::Value {
        if (groupId.empty()) {
          return jsi::Value::null();
        } else {
          return jsi::String::createFromUtf8(rt, groupId);
        }
      });
}

jsi::Value NativePhotoBurstDetection::getPhotosInGroup(jsi::Runtime &rt,
                                                       std::string groupId) {
  return executeAsync<std::vector<std::string>>(
      rt,
      [this, groupId]() -> std::vector<std::string> {
        return groupManager_->getPhotosInGroup(groupId);
      },
      [this](jsi::Runtime &rt, const std::vector<std::string> &photoIds) {
        return createStringArray(rt, photoIds);
      });
}

// 分组管理接口实现
jsi::Value NativePhotoBurstDetection::mergeGroups(jsi::Runtime &rt,
                                                  jsi::Array groupIds,
                                                  jsi::String newGroupId) {
  return executeAsync<std::string>(
      rt,
      [this, &rt, &groupIds, &newGroupId]() -> std::string {
        auto idList = parseStringArray(rt, groupIds);
        std::string newId = newGroupId.isUndefined() ? "" : newGroupId.utf8(rt);
        return groupManager_->mergeGroups(idList, newId);
      },
      [](jsi::Runtime &rt, const std::string &resultGroupId) {
        return jsi::String::createFromUtf8(rt, resultGroupId);
      });
}

jsi::Value NativePhotoBurstDetection::splitGroup(jsi::Runtime &rt,
                                                 std::string groupId,
                                                 jsi::Array photoGroups) {
  return executeAsync<std::vector<std::string>>(
      rt,
      [this, &rt, groupId, &photoGroups]() -> std::vector<std::string> {
        std::vector<std::vector<std::string>> groups;
        size_t length = photoGroups.size(rt);
        for (size_t i = 0; i < length; i++) {
          auto subArray =
              photoGroups.getValueAtIndex(rt, i).asObject(rt).asArray(rt);
          groups.push_back(parseStringArray(rt, subArray));
        }
        return groupManager_->splitGroup(groupId, groups);
      },
      [this](jsi::Runtime &rt, const std::vector<std::string> &newGroupIds) {
        return createStringArray(rt, newGroupIds);
      });
}

// 系统管理接口实现
jsi::Value NativePhotoBurstDetection::clearAll(jsi::Runtime &rt) {
  return executeAsync<bool>(
      rt,
      [this]() -> bool {
        groupManager_->clearAllGroups();
        cache_->clearAll();
        return true;
      },
      [](jsi::Runtime &rt, const bool &) { return jsi::Value::undefined(); });
}

jsi::Value NativePhotoBurstDetection::getPerformanceStats(jsi::Runtime &rt) {
  return executeAsync<PerformanceStats>(
      rt,
      [this]() -> PerformanceStats {
        PerformanceStats stats;
        stats.totalPhotos =
            static_cast<int32_t>(groupManager_->getPhotoCount());
        stats.totalGroups =
            static_cast<int32_t>(groupManager_->getGroupCount());
        stats.cacheHitRate = cache_->getCacheHitRate();
        // 这里可以添加更多统计信息
        return stats;
      },
      [this](jsi::Runtime &rt, const PerformanceStats &stats) {
        return createPerformanceStatsObject(rt, stats);
      });
}

// 配置管理接口实现
jsi::Value NativePhotoBurstDetection::updateConfig(jsi::Runtime &rt,
                                                   jsi::Object config) {
  return executeAsync<bool>(
      rt,
      [this, &rt, &config]() -> bool {
        auto configData = parseConfig(rt, config);
        currentConfig_ = configData;
        engine_->updateConfig(configData);
        incrementalUpdater_->updateConfig(configData);
        return true;
      },
      [](jsi::Runtime &rt, const bool &) { return jsi::Value::undefined(); });
}

jsi::Value NativePhotoBurstDetection::getConfig(jsi::Runtime &rt) {
  return executeAsync<BurstDetectionConfig>(
      rt, [this]() -> BurstDetectionConfig { return currentConfig_; },
      [this](jsi::Runtime &rt, const BurstDetectionConfig &config) {
        return createConfigObject(rt, config);
      });
}

// 缓存管理接口实现
jsi::Value NativePhotoBurstDetection::clearCache(jsi::Runtime &rt) {
  return executeAsync<bool>(
      rt,
      [this]() -> bool {
        cache_->clearAll();
        return true;
      },
      [](jsi::Runtime &rt, const bool &) { return jsi::Value::undefined(); });
}

jsi::Value NativePhotoBurstDetection::getCacheStats(jsi::Runtime &rt) {
  return executeAsync<BurstDetectionCache::CacheStats>(
      rt,
      [this]() -> BurstDetectionCache::CacheStats {
        return cache_->getStats();
      },
      [](jsi::Runtime &rt,
         const BurstDetectionCache::CacheStats &stats) -> jsi::Value {
        auto obj = jsi::Object(rt);
        obj.setProperty(rt, "totalCacheSize",
                        static_cast<double>(stats.totalRequests));
        obj.setProperty(rt, "hitRate", stats.hitRate);
        obj.setProperty(rt, "groupCacheSize",
                        static_cast<double>(stats.groupCacheSize));
        obj.setProperty(rt, "timestampCacheSize",
                        static_cast<double>(stats.timestampCacheSize));
        obj.setProperty(rt, "resultCacheSize",
                        static_cast<double>(stats.resultCacheSize));
        return obj;
      });
}

// 验证和维护接口实现
jsi::Value NativePhotoBurstDetection::validateConsistency(jsi::Runtime &rt) {
  return executeAsync<bool>(
      rt,
      [this]() -> bool {
        try {
          groupManager_->validateConsistency();
          return true;
        } catch (...) {
          return false;
        }
      },
      [](jsi::Runtime &rt, const bool &isValid) {
        return jsi::Value(isValid);
      });
}

jsi::Value NativePhotoBurstDetection::removeEmptyGroups(jsi::Runtime &rt) {
  return executeAsync<std::vector<std::string>>(
      rt,
      [this]() -> std::vector<std::string> {
        auto allGroups = groupManager_->getAllGroups();
        std::vector<std::string> removedGroupIds;

        for (const auto &group : allGroups) {
          if (group.photoIds.empty()) {
            groupManager_->removeGroup(group.groupId);
            removedGroupIds.push_back(group.groupId);
          }
        }

        return removedGroupIds;
      },
      [this](jsi::Runtime &rt, const std::vector<std::string> &removedIds) {
        return createStringArray(rt, removedIds);
      });
}

// 开发和调试接口实现
jsi::Value NativePhotoBurstDetection::generateTestData(jsi::Runtime &rt,
                                                       std::string scenario,
                                                       double photoCount) {
  return executeAsync<std::vector<PhotoInfo>>(
      rt,
      [this, scenario, photoCount]() -> std::vector<PhotoInfo> {
        int count = static_cast<int>(photoCount);
        if (scenario == "basic_burst") {
          return testDataGenerator_->generateBurstSequence(
              count, std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count());
        } else if (scenario == "mixed_photos") {
          std::vector<int> burstSizes = {3, 5, 7};
          std::vector<int64_t> timeGaps = {30000, 60000,
                                           120000}; // 30秒, 1分钟, 2分钟
          return testDataGenerator_->generateMixedScenario(burstSizes,
                                                           timeGaps);
        } else if (scenario == "performance_test") {
          return testDataGenerator_->generatePerformanceTestData(count,
                                                                 count / 20);
        }
        return {};
      },
      [this](jsi::Runtime &rt,
             const std::vector<PhotoInfo> &photos) -> jsi::Value {
        auto arr = jsi::Array(rt, photos.size());
        for (size_t i = 0; i < photos.size(); i++) {
          arr.setValueAtIndex(rt, i, createPhotoInfoObject(rt, photos[i]));
        }
        return arr;
      });
}

jsi::Value NativePhotoBurstDetection::exportGroupsToJson(jsi::Runtime &rt) {
  return executeAsync<std::string>(
      rt,
      [this]() -> std::string {
        // 简单的JSON导出实现
        // 实际项目中可以使用更完善的JSON库
        auto groups = groupManager_->getAllGroups();
        std::string json = "[";
        for (size_t i = 0; i < groups.size(); i++) {
          if (i > 0)
            json += ",";
          json += "{\"groupId\":\"" + groups[i].groupId + "\",";
          json +=
              "\"photoCount\":" + std::to_string(groups[i].photoCount) + ",";
          json += "\"type\":\"" + groups[i].getTypeString() + "\"}";
        }
        json += "]";
        return json;
      },
      [](jsi::Runtime &rt, const std::string &json) {
        return jsi::String::createFromUtf8(rt, json);
      });
}

jsi::Value
NativePhotoBurstDetection::importGroupsFromJson(jsi::Runtime &rt,
                                                std::string jsonData) {
  return executeAsync<bool>(
      rt,
      [this, jsonData]() -> bool {
        // 简单的JSON导入实现
        // 实际项目中需要完善的JSON解析
        // 这里只是占位实现
        return true;
      },
      [](jsi::Runtime &rt, const bool &) { return jsi::Value::undefined(); });
}

// JSI 转换工具方法实现
PhotoInfo NativePhotoBurstDetection::parsePhotoInfo(jsi::Runtime &rt,
                                                    const jsi::Object &obj) {
  PhotoInfo photo;

  photo.id = obj.getProperty(rt, "id").asString(rt).utf8(rt);
  photo.filePath = obj.getProperty(rt, "filePath").asString(rt).utf8(rt);
  photo.timestamp =
      static_cast<int64_t>(obj.getProperty(rt, "timestamp").asNumber());
  photo.fileName = obj.getProperty(rt, "fileName").asString(rt).utf8(rt);
  photo.width = static_cast<int32_t>(obj.getProperty(rt, "width").asNumber());
  photo.height = static_cast<int32_t>(obj.getProperty(rt, "height").asNumber());

  // 可选字段
  if (obj.hasProperty(rt, "fileCreationTime")) {
    photo.fileCreationTime = static_cast<int64_t>(
        obj.getProperty(rt, "fileCreationTime").asNumber());
  }

  if (obj.hasProperty(rt, "cameraModel")) {
    photo.cameraModel =
        obj.getProperty(rt, "cameraModel").asString(rt).utf8(rt);
  }

  if (obj.hasProperty(rt, "latitude")) {
    photo.latitude = obj.getProperty(rt, "latitude").asNumber();
  }

  if (obj.hasProperty(rt, "longitude")) {
    photo.longitude = obj.getProperty(rt, "longitude").asNumber();
  }

  if (obj.hasProperty(rt, "hashValue")) {
    photo.hashValue = obj.getProperty(rt, "hashValue").asString(rt).utf8(rt);
  }

  return photo;
}

BurstDetectionConfig
NativePhotoBurstDetection::parseConfig(jsi::Runtime &rt,
                                       const jsi::Object &obj) {
  BurstDetectionConfig config;

  if (obj.hasProperty(rt, "maxTimeGapMs")) {
    config.maxTimeGapMs =
        static_cast<int64_t>(obj.getProperty(rt, "maxTimeGapMs").asNumber());
  }

  if (obj.hasProperty(rt, "minBurstSize")) {
    config.minBurstSize =
        static_cast<int32_t>(obj.getProperty(rt, "minBurstSize").asNumber());
  }

  if (obj.hasProperty(rt, "enableGpsAnalysis")) {
    config.enableGpsAnalysis =
        obj.getProperty(rt, "enableGpsAnalysis").asBool();
  }

  if (obj.hasProperty(rt, "enableSimilarityCheck")) {
    config.enableSimilarityCheck =
        obj.getProperty(rt, "enableSimilarityCheck").asBool();
  }

  return config;
}

std::vector<PhotoInfo>
NativePhotoBurstDetection::parsePhotoArray(jsi::Runtime &rt,
                                           const jsi::Array &arr) {
  std::vector<PhotoInfo> photos;
  size_t length = arr.size(rt);
  photos.reserve(length);

  for (size_t i = 0; i < length; i++) {
    auto obj = arr.getValueAtIndex(rt, i).asObject(rt);
    photos.push_back(parsePhotoInfo(rt, obj));
  }

  return photos;
}

std::vector<std::string>
NativePhotoBurstDetection::parseStringArray(jsi::Runtime &rt,
                                            const jsi::Array &arr) {
  std::vector<std::string> strings;
  size_t length = arr.size(rt);
  strings.reserve(length);

  for (size_t i = 0; i < length; i++) {
    strings.push_back(arr.getValueAtIndex(rt, i).asString(rt).utf8(rt));
  }

  return strings;
}

// JSI 输出转换方法实现
jsi::Object
NativePhotoBurstDetection::createPhotoInfoObject(jsi::Runtime &rt,
                                                 const PhotoInfo &photo) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "id", jsi::String::createFromUtf8(rt, photo.id));
  obj.setProperty(rt, "filePath",
                  jsi::String::createFromUtf8(rt, photo.filePath));
  obj.setProperty(rt, "timestamp", static_cast<double>(photo.timestamp));
  obj.setProperty(rt, "fileName",
                  jsi::String::createFromUtf8(rt, photo.fileName));
  obj.setProperty(rt, "width", static_cast<double>(photo.width));
  obj.setProperty(rt, "height", static_cast<double>(photo.height));

  if (photo.fileCreationTime > 0) {
    obj.setProperty(rt, "fileCreationTime",
                    static_cast<double>(photo.fileCreationTime));
  }

  if (!photo.cameraModel.empty()) {
    obj.setProperty(rt, "cameraModel",
                    jsi::String::createFromUtf8(rt, photo.cameraModel));
  }

  if (photo.hasGpsInfo()) {
    obj.setProperty(rt, "latitude", photo.latitude);
    obj.setProperty(rt, "longitude", photo.longitude);
  }

  if (!photo.hashValue.empty()) {
    obj.setProperty(rt, "hashValue",
                    jsi::String::createFromUtf8(rt, photo.hashValue));
  }

  return obj;
}

jsi::Object
NativePhotoBurstDetection::createBurstGroupObject(jsi::Runtime &rt,
                                                  const BurstGroup &group) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "groupId",
                  jsi::String::createFromUtf8(rt, group.groupId));
  obj.setProperty(rt, "photoIds", createStringArray(rt, group.photoIds));
  obj.setProperty(rt, "startTime", static_cast<double>(group.startTime));
  obj.setProperty(rt, "endTime", static_cast<double>(group.endTime));
  obj.setProperty(rt, "photoCount", static_cast<double>(group.photoCount));
  obj.setProperty(rt, "representativePhotoId",
                  jsi::String::createFromUtf8(rt, group.representativePhotoId));
  obj.setProperty(rt, "type",
                  jsi::String::createFromUtf8(rt, group.getTypeString()));

  return obj;
}

jsi::Object NativePhotoBurstDetection::createDetectionResultObject(
    jsi::Runtime &rt, const BurstDetectionResult &result) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "groups", createBurstGroupArray(rt, result.groups));
  obj.setProperty(rt, "totalPhotos", static_cast<double>(result.totalPhotos));
  obj.setProperty(rt, "burstGroupCount",
                  static_cast<double>(result.burstGroupCount));
  obj.setProperty(rt, "singlePhotoCount",
                  static_cast<double>(result.singlePhotoCount));
  obj.setProperty(rt, "processingTimeMs",
                  static_cast<double>(result.processingTimeMs));

  return obj;
}

jsi::Object NativePhotoBurstDetection::createIncrementalUpdateResultObject(
    jsi::Runtime &rt, const IncrementalUpdateResult &result) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "newGroups", createBurstGroupArray(rt, result.newGroups));
  obj.setProperty(rt, "modifiedGroupIds",
                  createStringArray(rt, result.modifiedGroupIds));
  obj.setProperty(rt, "deletedGroupIds",
                  createStringArray(rt, result.deletedGroupIds));
  obj.setProperty(rt, "processingTimeMs",
                  static_cast<double>(result.processingTimeMs));

  return obj;
}

jsi::Object NativePhotoBurstDetection::createPerformanceStatsObject(
    jsi::Runtime &rt, const PerformanceStats &stats) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "totalPhotos", static_cast<double>(stats.totalPhotos));
  obj.setProperty(rt, "totalGroups", static_cast<double>(stats.totalGroups));
  obj.setProperty(rt, "cacheHitRate", stats.cacheHitRate);
  obj.setProperty(rt, "lastProcessingTimeMs",
                  static_cast<double>(stats.lastProcessingTimeMs));

  return obj;
}

jsi::Object NativePhotoBurstDetection::createConfigObject(
    jsi::Runtime &rt, const BurstDetectionConfig &config) {
  auto obj = jsi::Object(rt);

  obj.setProperty(rt, "maxTimeGapMs", static_cast<double>(config.maxTimeGapMs));
  obj.setProperty(rt, "minBurstSize", static_cast<double>(config.minBurstSize));
  obj.setProperty(rt, "enableGpsAnalysis", config.enableGpsAnalysis);
  obj.setProperty(rt, "enableSimilarityCheck", config.enableSimilarityCheck);

  return obj;
}

jsi::Array NativePhotoBurstDetection::createBurstGroupArray(
    jsi::Runtime &rt, const std::vector<BurstGroup> &groups) {
  auto arr = jsi::Array(rt, groups.size());

  for (size_t i = 0; i < groups.size(); i++) {
    arr.setValueAtIndex(rt, i, createBurstGroupObject(rt, groups[i]));
  }

  return arr;
}

jsi::Array NativePhotoBurstDetection::createStringArray(
    jsi::Runtime &rt, const std::vector<std::string> &strings) {
  auto arr = jsi::Array(rt, strings.size());

  for (size_t i = 0; i < strings.size(); i++) {
    arr.setValueAtIndex(rt, i, jsi::String::createFromUtf8(rt, strings[i]));
  }

  return arr;
}

// Promise 工具方法实现
jsi::Value NativePhotoBurstDetection::createResolvedPromise(jsi::Runtime &rt,
                                                            jsi::Value value) {
  auto promiseConstructor = rt.global().getPropertyAsFunction(rt, "Promise");
  auto resolve = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "resolve"), 1,
      [value = std::move(value)](jsi::Runtime &rt, const jsi::Value &,
                                 const jsi::Value *args, size_t) mutable {
        args[0].asObject(rt).asFunction(rt).call(rt, value);
        return jsi::Value::undefined();
      });

  return promiseConstructor.callAsConstructor(rt, resolve);
}

jsi::Value
NativePhotoBurstDetection::createRejectedPromise(jsi::Runtime &rt,
                                                 const std::string &error) {
  auto promiseConstructor = rt.global().getPropertyAsFunction(rt, "Promise");
  auto reject = jsi::Function::createFromHostFunction(
      rt, jsi::PropNameID::forAscii(rt, "reject"), 1,
      [error](jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args,
              size_t) {
        auto errorObj = jsi::Object(rt);
        errorObj.setProperty(rt, "message",
                             jsi::String::createFromUtf8(rt, error));
        args[1].asObject(rt).asFunction(rt).call(rt, errorObj);
        return jsi::Value::undefined();
      });

  return promiseConstructor.callAsConstructor(rt, reject);
}

// 异步执行工具实现
template <typename T>
jsi::Value NativePhotoBurstDetection::executeAsync(
    jsi::Runtime &rt, std::function<T()> task,
    std::function<jsi::Value(jsi::Runtime &, const T &)> resultConverter) {

  auto promiseConstructor = rt.global().getPropertyAsFunction(rt, "Promise");

  return promiseConstructor.callAsConstructor(
      rt, jsi::Function::createFromHostFunction(
              rt, jsi::PropNameID::forAscii(rt, "executor"), 2,
              [this, task = std::move(task),
               resultConverter = std::move(resultConverter)](
                  jsi::Runtime &rt, const jsi::Value &, const jsi::Value *args,
                  size_t) mutable {
                auto resolve = args[0].asObject(rt).asFunction(rt);
                auto reject = args[1].asObject(rt).asFunction(rt);

                try {
                  if (!isInitialized_) {
                    reject.call(
                        rt,
                        jsi::String::createFromUtf8(
                            rt, "NativePhotoBurstDetection not initialized"));
                    return jsi::Value::undefined();
                  }

                  auto result = task();
                  auto jsResult = resultConverter(rt, result);
                  resolve.call(rt, jsResult);

                } catch (const std::exception &e) {
                  reject.call(rt, jsi::String::createFromUtf8(rt, e.what()));
                } catch (...) {
                  reject.call(rt, jsi::String::createFromUtf8(
                                      rt, "Unknown error occurred"));
                }

                return jsi::Value::undefined();
              }));
}

// 输入验证实现
bool NativePhotoBurstDetection::validatePhotoInfo(const PhotoInfo &photo) {
  return !photo.id.empty() && !photo.filePath.empty() && photo.timestamp > 0 &&
         photo.width > 0 && photo.height > 0;
}

bool NativePhotoBurstDetection::validateConfig(
    const BurstDetectionConfig &config) {
  return config.maxTimeGapMs > 0 && config.minBurstSize >= 2;
}

} // namespace facebook::react
