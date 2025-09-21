#pragma once

#include <AppSpecsJSI.h>
#include <memory>
#include <string>
#include <vector>

#include "BurstDetectionCache.h"
#include "BurstDetectionEngine.h"
#include "IncrementalUpdater.h"
#include "PhotoGroupManager.h"
#include "TestDataGenerator.h"
#include "Types.h"

namespace facebook::react {

class NativePhotoBurstDetection
    : public NativePhotoBurstDetectionCxxSpec<NativePhotoBurstDetection> {
public:
  NativePhotoBurstDetection(std::shared_ptr<CallInvoker> jsInvoker);
  ~NativePhotoBurstDetection();

  // 核心检测接口
  jsi::Value detectBursts(jsi::Runtime &rt, jsi::Array photos,
                          jsi::Object config);

  // 增量更新接口
  jsi::Value addPhotos(jsi::Runtime &rt, jsi::Array photos);
  jsi::Value removePhotos(jsi::Runtime &rt, jsi::Array photoIds);
  jsi::Value updatePhotos(jsi::Runtime &rt, jsi::Array photos);
  jsi::Value batchUpdate(jsi::Runtime &rt, jsi::Array addedPhotos,
                         jsi::Array removedPhotoIds, jsi::Array updatedPhotos);

  // 查询接口
  jsi::Value getGroupDetails(jsi::Runtime &rt, std::string groupId);
  jsi::Value getAllGroups(jsi::Runtime &rt);
  jsi::Value getPhotoGroup(jsi::Runtime &rt, std::string photoId);
  jsi::Value getPhotosInGroup(jsi::Runtime &rt, std::string groupId);

  // 分组管理接口
  jsi::Value mergeGroups(jsi::Runtime &rt, jsi::Array groupIds,
                         jsi::String newGroupId);
  jsi::Value splitGroup(jsi::Runtime &rt, std::string groupId,
                        jsi::Array photoGroups);

  // 系统管理接口
  jsi::Value clearAll(jsi::Runtime &rt);
  jsi::Value getPerformanceStats(jsi::Runtime &rt);

  // 配置管理接口
  jsi::Value updateConfig(jsi::Runtime &rt, jsi::Object config);
  jsi::Value getConfig(jsi::Runtime &rt);

  // 缓存管理接口
  jsi::Value clearCache(jsi::Runtime &rt);
  jsi::Value getCacheStats(jsi::Runtime &rt);

  // 验证和维护接口
  jsi::Value validateConsistency(jsi::Runtime &rt);
  jsi::Value removeEmptyGroups(jsi::Runtime &rt);

  // 开发和调试接口
  jsi::Value generateTestData(jsi::Runtime &rt, std::string scenario,
                              double photoCount);
  jsi::Value exportGroupsToJson(jsi::Runtime &rt);
  jsi::Value importGroupsFromJson(jsi::Runtime &rt, std::string jsonData);

private:
  // 核心组件
  std::shared_ptr<BurstDetectionEngine> engine_;
  std::shared_ptr<PhotoGroupManager> groupManager_;
  std::shared_ptr<BurstDetectionCache> cache_;
  std::shared_ptr<IncrementalUpdater> incrementalUpdater_;
  std::shared_ptr<TestDataGenerator> testDataGenerator_;

  // 当前配置
  BurstDetectionConfig currentConfig_;

  // JSI 转换工具方法
  PhotoInfo parsePhotoInfo(jsi::Runtime &rt, const jsi::Object &obj);
  BurstDetectionConfig parseConfig(jsi::Runtime &rt, const jsi::Object &obj);
  std::vector<PhotoInfo> parsePhotoArray(jsi::Runtime &rt,
                                         const jsi::Array &arr);
  std::vector<std::string> parseStringArray(jsi::Runtime &rt,
                                            const jsi::Array &arr);

  // JSI 输出转换方法
  jsi::Object createPhotoInfoObject(jsi::Runtime &rt, const PhotoInfo &photo);
  jsi::Object createBurstGroupObject(jsi::Runtime &rt, const BurstGroup &group);
  jsi::Object createDetectionResultObject(jsi::Runtime &rt,
                                          const BurstDetectionResult &result);
  jsi::Object
  createIncrementalUpdateResultObject(jsi::Runtime &rt,
                                      const IncrementalUpdateResult &result);
  jsi::Object createPerformanceStatsObject(jsi::Runtime &rt,
                                           const PerformanceStats &stats);
  jsi::Object createConfigObject(jsi::Runtime &rt,
                                 const BurstDetectionConfig &config);

  jsi::Array createBurstGroupArray(jsi::Runtime &rt,
                                   const std::vector<BurstGroup> &groups);
  jsi::Array createStringArray(jsi::Runtime &rt,
                               const std::vector<std::string> &strings);

  // Promise 工具方法
  jsi::Value createResolvedPromise(jsi::Runtime &rt, jsi::Value value);
  jsi::Value createRejectedPromise(jsi::Runtime &rt, const std::string &error);

  // 错误处理
  void handleException(jsi::Runtime &rt, const std::exception &e,
                       const std::string &context);

  // 输入验证
  bool validatePhotoInfo(const PhotoInfo &photo);
  bool validateConfig(const BurstDetectionConfig &config);

  // 异步执行工具
  template <typename T>
  jsi::Value executeAsync(
      jsi::Runtime &rt, std::function<T()> task,
      std::function<jsi::Value(jsi::Runtime &, const T &)> resultConverter);

  // 初始化和清理
  void initializeComponents();
  void cleanupComponents();

  // 线程安全
  std::mutex operationMutex_;
  bool isInitialized_;

  // 性能监控
  void logPerformanceMetrics(const std::string &operation,
                             int64_t processingTime);
};

} // namespace facebook::react
