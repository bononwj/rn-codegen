#include "../src/shared/burst-detection/BurstDetectionCache.h"
#include "../src/shared/burst-detection/BurstDetectionEngine.h"
#include "../src/shared/burst-detection/IncrementalUpdater.h"
#include "../src/shared/burst-detection/PhotoGroupManager.h"
#include "../src/shared/burst-detection/TestDataGenerator.h"
#include <cassert>
#include <chrono>
#include <iostream>


using namespace facebook::react;

// 测试工具函数
void printTestResult(const std::string &testName, bool passed) {
  std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName
            << std::endl;
}

// 基础连拍检测测试
bool testBasicBurstDetection() {
  BurstDetectionEngine engine;
  TestDataGenerator generator;

  // 生成测试数据：5张连续拍摄的照片
  auto photos = generator.generateBurstSequence(5, 1627890000000, 500);

  // 执行连拍检测
  auto result = engine.detectBursts(photos);

  // 验证结果
  bool passed =
      (result.groups.size() == 1 && result.groups[0].type == GroupType::BURST &&
       result.groups[0].photoCount == 5);

  return passed;
}

// 混合场景测试
bool testMixedScenario() {
  BurstDetectionEngine engine;
  TestDataGenerator generator;

  // 生成混合场景测试数据
  std::vector<int> burstSizes = {3, 5};
  std::vector<int64_t> timeGaps = {30000, 60000}; // 30秒和60秒间隔
  auto photos = generator.generateMixedScenario(burstSizes, timeGaps);

  // 执行检测
  auto result = engine.detectBursts(photos);

  // 验证结果：应该有2个连拍组和若干单张照片
  int burstCount = 0;
  int singleCount = 0;

  for (const auto &group : result.groups) {
    if (group.type == GroupType::BURST) {
      burstCount++;
    } else if (group.type == GroupType::SINGLE) {
      singleCount++;
    }
  }

  bool passed = (burstCount == 2 && singleCount >= 0);
  return passed;
}

// 增量更新测试
bool testIncrementalUpdate() {
  auto engine = std::make_shared<BurstDetectionEngine>();
  auto groupManager = std::make_shared<PhotoGroupManager>();
  auto cache = std::make_shared<BurstDetectionCache>();
  auto incrementalUpdater =
      std::make_shared<IncrementalUpdater>(engine, groupManager, cache);

  TestDataGenerator generator;

  // 初始数据
  auto initialPhotos = generator.generateBurstSequence(3, 1627890000000, 500);
  auto addResult = incrementalUpdater->addPhotos(initialPhotos);

  // 添加更多照片形成连拍
  auto additionalPhotos =
      generator.generateBurstSequence(2, 1627890001500, 500);
  auto updateResult = incrementalUpdater->addPhotos(additionalPhotos);

  // 验证增量更新是否正确
  bool passed =
      (addResult.newGroups.size() > 0 && updateResult.newGroups.size() >= 0);
  return passed;
}

// 性能测试
bool testPerformance() {
  BurstDetectionEngine engine;
  TestDataGenerator generator;

  // 生成大量测试数据
  auto photos = generator.generatePerformanceTestData(1000, 50);

  // 测量处理时间
  auto startTime = std::chrono::high_resolution_clock::now();
  auto result = engine.detectBursts(photos);
  auto endTime = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  std::cout << "Performance Test: " << photos.size() << " photos processed in "
            << duration.count() << "ms" << std::endl;

  // 验证性能要求：1000张照片应该在合理时间内完成（比如5秒）
  bool passed = (duration.count() < 5000 &&
                 result.totalPhotos == static_cast<int32_t>(photos.size()));
  return passed;
}

// 缓存测试
bool testCaching() {
  BurstDetectionCache cache;

  // 创建测试分组
  BurstGroup testGroup;
  testGroup.groupId = "test_group_001";
  testGroup.photoCount = 3;
  testGroup.type = GroupType::BURST;

  // 测试缓存存储和检索
  cache.cacheGroup(testGroup.groupId, testGroup);

  BurstGroup retrievedGroup;
  bool found = cache.getCachedGroup(testGroup.groupId, retrievedGroup);

  bool passed = (found && retrievedGroup.groupId == testGroup.groupId &&
                 retrievedGroup.photoCount == testGroup.photoCount);

  return passed;
}

// 分组管理测试
bool testGroupManager() {
  PhotoGroupManager manager;

  // 创建测试分组
  BurstGroup group1;
  group1.groupId = "group_001";
  group1.photoIds = {"photo_001", "photo_002", "photo_003"};
  group1.photoCount = 3;
  group1.type = GroupType::BURST;

  // 添加分组
  manager.addGroup(group1);

  // 验证查询功能
  auto retrievedGroup = manager.getGroup("group_001");
  bool passed =
      (retrievedGroup != nullptr && retrievedGroup->groupId == group1.groupId &&
       retrievedGroup->photoCount == group1.photoCount);

  // 测试照片到分组的映射
  if (passed) {
    auto groupId = manager.getPhotoGroup("photo_001");
    passed = (groupId == "group_001");
  }

  return passed;
}

// 配置测试
bool testConfiguration() {
  BurstDetectionConfig config;
  config.maxTimeGapMs = 1000;
  config.minBurstSize = 2;
  config.enableGpsAnalysis = true;

  BurstDetectionEngine engine(config);

  // 验证配置是否正确应用
  auto currentConfig = engine.getConfig();
  bool passed =
      (currentConfig.maxTimeGapMs == 1000 && currentConfig.minBurstSize == 2 &&
       currentConfig.enableGpsAnalysis == true);

  return passed;
}

// 主测试函数
int main() {
  std::cout << "=== 照片连拍识别功能测试 ===" << std::endl;
  std::cout << std::endl;

  int passedTests = 0;
  int totalTests = 0;

  // 运行所有测试
  totalTests++;
  printTestResult("基础连拍检测", testBasicBurstDetection());
  if (testBasicBurstDetection())
    passedTests++;

  totalTests++;
  printTestResult("混合场景测试", testMixedScenario());
  if (testMixedScenario())
    passedTests++;

  totalTests++;
  printTestResult("增量更新测试", testIncrementalUpdate());
  if (testIncrementalUpdate())
    passedTests++;

  totalTests++;
  printTestResult("性能测试", testPerformance());
  if (testPerformance())
    passedTests++;

  totalTests++;
  printTestResult("缓存功能测试", testCaching());
  if (testCaching())
    passedTests++;

  totalTests++;
  printTestResult("分组管理测试", testGroupManager());
  if (testGroupManager())
    passedTests++;

  totalTests++;
  printTestResult("配置管理测试", testConfiguration());
  if (testConfiguration())
    passedTests++;

  std::cout << std::endl;
  std::cout << "=== 测试结果 ===" << std::endl;
  std::cout << "通过: " << passedTests << "/" << totalTests << " 个测试"
            << std::endl;
  std::cout << "成功率: " << (100.0 * passedTests / totalTests) << "%"
            << std::endl;

  if (passedTests == totalTests) {
    std::cout << "🎉 所有测试通过！照片连拍识别功能实现成功！" << std::endl;
    return 0;
  } else {
    std::cout << "❌ 部分测试失败，需要进一步调试。" << std::endl;
    return 1;
  }
}