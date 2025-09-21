#pragma once

#include "Types.h"
#include <random>

namespace facebook::react {

class TestDataGenerator {
public:
  TestDataGenerator();
  ~TestDataGenerator();

  // 基础连拍序列生成
  static std::vector<PhotoInfo>
  generateBurstSequence(int count, int64_t startTime, int intervalMs = 500,
                        const std::string &baseId = "IMG",
                        const std::string &cameraModel = "iPhone 12 Pro");

  // 混合场景生成
  static std::vector<PhotoInfo>
  generateMixedScenario(const std::vector<int> &burstSizes,
                        const std::vector<int64_t> &timeGaps,
                        int64_t startTime = 0);

  // 大量数据性能测试
  static std::vector<PhotoInfo>
  generatePerformanceTestData(int totalPhotos, int burstGroupCount,
                              int avgBurstSize = 5, int timeSpanDays = 365);

  // 特定场景生成
  static std::vector<PhotoInfo>
  generateTimeClusterScenario(int totalPhotos, int clusterCount,
                              int64_t clusterTimeSpanMs = 10000);

  static std::vector<PhotoInfo> generateGpsTestData(int photoCount,
                                                    double centerLat = 39.9042,
                                                    double centerLng = 116.4074,
                                                    double radiusKm = 1.0);

  static std::vector<PhotoInfo>
  generateFileNamePatternData(int photoCount,
                              const std::vector<std::string> &patterns = {
                                  "IMG_%04d.jpg", "DSC_%05d.jpg"});

  // 边界情况测试数据
  static std::vector<PhotoInfo> generateEdgeCaseData();
  static std::vector<PhotoInfo> generateEmptyData();
  static std::vector<PhotoInfo> generateSinglePhotoData();
  static std::vector<PhotoInfo> generateIdenticalTimestampData(int count);
  static std::vector<PhotoInfo> generateLargeTimeGapData();

  // 质量测试数据
  static std::vector<PhotoInfo>
  generateQualityTestData(int burstCount, int avgBurstSize,
                          double noiseLevel = 0.1); // 噪声水平：0.0-1.0

  // 配置特定测试数据
  static std::vector<PhotoInfo>
  generateDataForConfig(const BurstDetectionConfig &config,
                        int totalPhotos = 100);

  // 预定义测试场景
  enum class TestScenario {
    BASIC_BURST,
    MIXED_PHOTOS,
    PERFORMANCE_TEST,
    GPS_CLUSTER,
    FILENAME_PATTERN,
    EDGE_CASES,
    QUALITY_TEST,
    CONFIG_SPECIFIC
  };

  static std::vector<PhotoInfo>
  generateScenarioData(TestScenario scenario,
                       const std::map<std::string, int> &params = {});

  // 测试数据验证
  struct TestDataValidation {
    bool isValid;
    int expectedBurstGroups;
    int expectedSinglePhotos;
    std::vector<std::string> validationErrors;
    std::vector<std::pair<int, int>> expectedBurstRanges; // start index, count
  };

  static TestDataValidation
  validateTestData(const std::vector<PhotoInfo> &photos,
                   const BurstDetectionConfig &config = BurstDetectionConfig());

  // JSON导出支持
  static std::string exportToJson(const std::vector<PhotoInfo> &photos,
                                  const std::string &scenarioName = "",
                                  const std::string &description = "");

  static std::vector<PhotoInfo> importFromJson(const std::string &jsonData);

private:
  static std::mt19937 &getRandomGenerator();

  // 工具方法
  static std::string generatePhotoId(const std::string &prefix, int index);
  static std::string generateFileName(const std::string &pattern, int index);
  static std::string generateFilePath(const std::string &fileName);
  static std::string selectRandomCameraModel();
  static std::pair<int, int> selectRandomDimensions();

  // 时间戳生成
  static int64_t generateRandomTimestamp(int64_t baseTime, int64_t range);
  static int64_t addJitter(int64_t baseTime, int jitterMs);

  // GPS坐标生成
  static std::pair<double, double>
  generateRandomGpsNear(double centerLat, double centerLng, double radiusKm);

  // 文件名模式
  static std::vector<std::string> getCommonFileNamePatterns();
  static std::vector<std::string> getCommonCameraModels();

  // 预设场景数据
  static std::vector<PhotoInfo> createBasicBurstScenario();
  static std::vector<PhotoInfo> createMixedPhotoScenario();
  static std::vector<PhotoInfo> createPerformanceScenario();
  static std::vector<PhotoInfo> createGpsClusterScenario();
  static std::vector<PhotoInfo> createFileNamePatternScenario();
  static std::vector<PhotoInfo> createEdgeCaseScenario();
  static std::vector<PhotoInfo> createQualityTestScenario();

  // 验证辅助方法
  static int countExpectedBursts(const std::vector<PhotoInfo> &photos,
                                 const BurstDetectionConfig &config);
  static std::vector<std::pair<int, int>>
  findExpectedBurstRanges(const std::vector<PhotoInfo> &photos,
                          const BurstDetectionConfig &config);

  // 常量
  static constexpr int64_t MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000L;
  static constexpr int64_t DEFAULT_BASE_TIMESTAMP =
      1627890000000L; // 2021-08-01
  static constexpr int DEFAULT_IMAGE_WIDTH = 4032;
  static constexpr int DEFAULT_IMAGE_HEIGHT = 3024;
  static constexpr double EARTH_RADIUS_KM = 6371.0;
};

} // namespace facebook::react