# 照片连拍识别功能技术设计方案

## 1. 功能概述

### 1.1 功能目标

实现一个高性能的照片连拍识别模块，能够：

- 对提供的照片信息进行连拍分组
- 支持增量更新，处理新添加的照片
- 提供 UI 友好的分组结果，便于列表展示
- 极致性能优化，支持大量照片处理

### 1.2 核心特性

- **智能分组算法**：基于时间戳、EXIF 信息、文件名等多维度识别连拍
- **增量处理**：支持新照片的实时分组更新
- **性能优化**：使用 C++实现核心算法，采用高效的数据结构和缓存策略
- **跨平台支持**：iOS 和 Android 双平台支持

## 2. 技术架构

### 2.1 整体架构

```
┌─────────────────────────────────────────┐
│              React Native               │
│         (JavaScript/TypeScript)         │
├─────────────────────────────────────────┤
│           TurboModule Bridge            │
├─────────────────────────────────────────┤
│        C++ Core Algorithm Module       │
│   - BurstDetectionEngine               │
│   - PhotoGroupManager                  │
│   - IncrementalUpdater                 │
├─────────────────────────────────────────┤
│          Platform Specific             │
│     iOS (Objective-C++) / Android       │
└─────────────────────────────────────────┘
```

### 2.2 模块设计

#### 2.2.1 NativePhotoBurstDetection (主模块)

- **初始化分组**：一次性处理大量照片
- **增量更新**：处理新增照片
- **分组查询**：获取分组结果
- **性能统计**：提供处理时间等性能指标

#### 2.2.2 核心 C++类设计

```cpp
// 照片信息结构
struct PhotoInfo {
    std::string id;              // 唯一标识
    std::string filePath;        // 文件路径
    int64_t timestamp;          // 拍摄时间戳（毫秒）
    int64_t fileCreationTime;   // 文件创建时间
    std::string fileName;       // 文件名
    int32_t width;              // 图片宽度
    int32_t height;             // 图片高度
    std::string cameraModel;    // 相机型号
    double latitude;            // GPS纬度 (可选)
    double longitude;           // GPS经度 (可选)
    std::string hashValue;      // 文件hash值 (可选)
};

// 连拍分组结构
struct BurstGroup {
    std::string groupId;                // 分组ID
    std::vector<std::string> photoIds;  // 照片ID列表
    int64_t startTime;                  // 连拍开始时间
    int64_t endTime;                    // 连拍结束时间
    int32_t photoCount;                 // 照片数量
    std::string representativePhotoId;  // 代表照片ID (通常是第一张或中间张)
    GroupType type;                     // 分组类型：BURST, SIMILAR, SINGLE
};

// 分组结果
struct BurstDetectionResult {
    std::vector<BurstGroup> groups;     // 所有分组
    int32_t totalPhotos;                // 总照片数量
    int32_t burstGroupCount;            // 连拍分组数量
    int32_t singlePhotoCount;           // 单张照片数量
    int64_t processingTimeMs;           // 处理耗时
};
```

## 3. 算法设计

### 3.1 连拍识别算法

#### 3.1.1 多层级判断机制

**第一层：时间窗口过滤**

- 连续照片时间间隔 < 10 秒 才考虑为潜在连拍
- 使用滑动窗口算法，提高效率

**第二层：精确时间分析**

- 连拍照片间隔：通常 < 2 秒
- 同一连拍序列内时间间隔稳定性分析
- 处理时钟不准确的情况

**第三层：EXIF 信息验证**

- 相机型号一致性
- 拍摄参数相似性（ISO、光圈、快门等）
- 图片尺寸一致性

**第四层：文件名模式识别**

- 连续文件名模式（如 IMG_001.jpg, IMG_002.jpg）
- 相机厂商特定命名规律
- 时间戳命名模式

#### 3.1.2 算法复杂度

- 时间复杂度：O(n log n) - 主要是排序开销
- 空间复杂度：O(n) - 线性存储
- 增量更新：O(log n) - 仅处理新增照片的潜在影响

### 3.2 增量更新策略

```cpp
class IncrementalUpdater {
public:
    // 添加新照片并返回受影响的分组
    std::vector<std::string> addPhotos(const std::vector<PhotoInfo>& newPhotos);

    // 移除照片并返回受影响的分组
    std::vector<std::string> removePhotos(const std::vector<std::string>& photoIds);

private:
    // 查找需要重新评估的时间窗口
    std::pair<int64_t, int64_t> findAffectedTimeWindow(const PhotoInfo& photo);

    // 局部重新分组
    void reprocessTimeWindow(int64_t startTime, int64_t endTime);
};
```

## 4. 性能优化策略

### 4.1 数据结构优化

- **时间索引**：使用 `std::map<int64_t, std::vector<PhotoInfo>>` 按时间快速定位
- **空间索引**：GPS 信息的 KD 树索引（可选）
- **哈希缓存**：照片特征信息缓存

### 4.2 算法优化

- **多线程处理**：将大数据集分片并行处理
- **懒加载**：分组结果按需计算详细信息
- **内存池**：避免频繁内存分配

### 4.3 缓存策略

```cpp
class BurstDetectionCache {
private:
    std::unordered_map<std::string, BurstGroup> groupCache;
    std::unordered_map<std::string, int64_t> photoTimestampCache;
    LRUCache<std::string, BurstDetectionResult> resultCache;
public:
    void invalidatePhotoGroups(const std::vector<std::string>& photoIds);
    BurstDetectionResult* getCachedResult(const std::string& key);
};
```

## 5. API 设计

### 5.1 TypeScript 接口定义

```typescript
// src/specs/NativePhotoBurstDetection.ts
import { TurboModule, TurboModuleRegistry } from 'react-native';

export interface PhotoInfo {
  id: string;
  filePath: string;
  timestamp: number; // 毫秒时间戳
  fileCreationTime?: number; // 文件创建时间
  fileName: string;
  width: number;
  height: number;
  cameraModel?: string;
  latitude?: number;
  longitude?: number;
  hashValue?: string;
}

export interface BurstGroup {
  groupId: string;
  photoIds: string[];
  startTime: number;
  endTime: number;
  photoCount: number;
  representativePhotoId: string;
  type: 'BURST' | 'SIMILAR' | 'SINGLE';
}

export interface BurstDetectionResult {
  groups: BurstGroup[];
  totalPhotos: number;
  burstGroupCount: number;
  singlePhotoCount: number;
  processingTimeMs: number;
}

export interface BurstDetectionConfig {
  maxTimeGapMs?: number; // 最大时间间隔，默认2000ms
  minBurstSize?: number; // 最小连拍数量，默认3张
  enableGpsAnalysis?: boolean; // 是否启用GPS分析，默认false
  enableSimilarityCheck?: boolean; // 是否启用相似度检查，默认false
}

export interface Spec extends TurboModule {
  // 初始化并分析所有照片
  readonly detectBursts: (
    photos: PhotoInfo[],
    config?: BurstDetectionConfig,
  ) => Promise<BurstDetectionResult>;

  // 增量添加新照片
  readonly addPhotos: (photos: PhotoInfo[]) => Promise<{
    newGroups: BurstGroup[];
    modifiedGroupIds: string[];
    processingTimeMs: number;
  }>;

  // 移除照片
  readonly removePhotos: (photoIds: string[]) => Promise<{
    modifiedGroupIds: string[];
    deletedGroupIds: string[];
  }>;

  // 获取指定分组的详细信息
  readonly getGroupDetails: (groupId: string) => Promise<BurstGroup | null>;

  // 获取所有分组的简要信息（用于列表展示）
  readonly getAllGroups: () => Promise<BurstGroup[]>;

  // 清除所有数据和缓存
  readonly clearAll: () => Promise<void>;

  // 获取性能统计信息
  readonly getPerformanceStats: () => Promise<{
    totalPhotos: number;
    totalGroups: number;
    cacheHitRate: number;
    lastProcessingTimeMs: number;
  }>;
}

export default TurboModuleRegistry.getEnforcing<Spec>(
  'NativePhotoBurstDetection',
);
```

### 5.2 C++接口实现

```cpp
// src/shared/burst-detection/NativePhotoBurstDetection.h
#pragma once

#include <AppSpecsJSI.h>
#include <memory>
#include <vector>
#include <string>

namespace facebook::react {

class NativePhotoBurstDetection : public NativePhotoBurstDetectionCxxSpec<NativePhotoBurstDetection> {
public:
    NativePhotoBurstDetection(std::shared_ptr<CallInvoker> jsInvoker);

    jsi::Value detectBursts(jsi::Runtime &rt, jsi::Array photos, jsi::Object config);
    jsi::Value addPhotos(jsi::Runtime &rt, jsi::Array photos);
    jsi::Value removePhotos(jsi::Runtime &rt, jsi::Array photoIds);
    jsi::Value getGroupDetails(jsi::Runtime &rt, std::string groupId);
    jsi::Value getAllGroups(jsi::Runtime &rt);
    jsi::Value clearAll(jsi::Runtime &rt);
    jsi::Value getPerformanceStats(jsi::Runtime &rt);

private:
    std::unique_ptr<BurstDetectionEngine> engine_;
    std::unique_ptr<PhotoGroupManager> groupManager_;
    std::unique_ptr<IncrementalUpdater> incrementalUpdater_;
};

} // namespace facebook::react
```

## 6. 测试数据设计

### 6.1 测试场景设计

#### 6.1.1 基础连拍场景

```json
{
  "scenario": "basic_burst",
  "description": "5张连续拍摄的照片",
  "photos": [
    {
      "id": "IMG_001",
      "fileName": "IMG_001.jpg",
      "timestamp": 1627890000000,
      "width": 4032,
      "height": 3024,
      "cameraModel": "iPhone 12 Pro"
    },
    {
      "id": "IMG_002",
      "fileName": "IMG_002.jpg",
      "timestamp": 1627890000500,
      "width": 4032,
      "height": 3024,
      "cameraModel": "iPhone 12 Pro"
    }
    // ... 更多照片
  ],
  "expectedGroups": [
    {
      "type": "BURST",
      "photoCount": 5,
      "photoIds": ["IMG_001", "IMG_002", "IMG_003", "IMG_004", "IMG_005"]
    }
  ]
}
```

#### 6.1.2 混合场景

```json
{
  "scenario": "mixed_photos",
  "description": "包含连拍、单张、时间间隔大的照片",
  "photos": [
    // 第一组连拍 (3张)
    // 单张照片 (时间间隔大)
    // 第二组连拍 (7张)
    // 单张照片
  ],
  "expectedGroups": [
    { "type": "BURST", "photoCount": 3 },
    { "type": "SINGLE", "photoCount": 1 },
    { "type": "BURST", "photoCount": 7 },
    { "type": "SINGLE", "photoCount": 1 }
  ]
}
```

#### 6.1.3 性能测试场景

```json
{
  "scenario": "performance_test",
  "description": "大量照片性能测试",
  "photoCount": 10000,
  "burstGroups": 50,
  "avgBurstSize": 8,
  "timeSpanDays": 365,
  "expectedProcessingTimeMs": 500
}
```

### 6.2 测试数据生成器

```cpp
// src/shared/burst-detection/TestDataGenerator.h
class TestDataGenerator {
public:
    // 生成基础连拍测试数据
    static std::vector<PhotoInfo> generateBurstSequence(
        int count,
        int64_t startTime,
        int intervalMs = 500
    );

    // 生成混合场景测试数据
    static std::vector<PhotoInfo> generateMixedScenario(
        const std::vector<int>& burstSizes,
        const std::vector<int64_t>& timeGaps
    );

    // 生成大量数据用于性能测试
    static std::vector<PhotoInfo> generatePerformanceTestData(
        int totalPhotos,
        int burstGroupCount
    );
};
```

## 7. UI 展示优化

### 7.1 分组展示策略

```typescript
// UI友好的分组信息
interface UIBurstGroup {
  groupId: string;
  type: 'BURST' | 'SINGLE';
  displayTitle: string; // "连拍 (5张)" 或 "单张照片"
  thumbnailPhotoId: string; // 缩略图照片ID
  photoCount: number;
  timeRange: string; // "2023-08-01 14:30-14:32"
  isExpanded?: boolean; // 是否展开显示
  childPhotos?: PhotoInfo[]; // 展开时显示的子照片
}
```

### 7.2 列表渲染优化

- **懒加载**：按需加载分组详情
- **虚拟列表**：支持大量分组的流畅滚动
- **缩略图缓存**：预加载代表照片的缩略图

## 8. 实现计划

### 8.1 开发阶段

**阶段 1：核心算法实现 (2-3 天)**

- 实现基础的连拍检测算法
- 完成 C++核心类的设计和实现
- 单元测试覆盖

**阶段 2：TurboModule 集成 (1-2 天)**

- 实现 React Native 接口
- JSI 绑定实现
- TypeScript 类型定义

**阶段 3：平台适配 (1-2 天)**

- iOS 平台集成
- Android 平台集成
- 平台特定优化

**阶段 4：性能优化和测试 (1-2 天)**

- 性能基准测试
- 内存优化
- 缓存策略实现

**阶段 5：增量更新功能 (1-2 天)**

- 增量算法实现
- 性能验证
- 集成测试

### 8.2 文件结构

```
src/shared/burst-detection/
├── NativePhotoBurstDetection.h
├── NativePhotoBurstDetection.cpp
├── BurstDetectionEngine.h
├── BurstDetectionEngine.cpp
├── PhotoGroupManager.h
├── PhotoGroupManager.cpp
├── IncrementalUpdater.h
├── IncrementalUpdater.cpp
├── BurstDetectionCache.h
├── BurstDetectionCache.cpp
├── TestDataGenerator.h
├── TestDataGenerator.cpp
└── Types.h

src/specs/
└── NativePhotoBurstDetection.ts

tests/
├── test-data/
│   ├── basic-burst.json
│   ├── mixed-scenario.json
│   └── performance-test.json
└── BurstDetectionTest.cpp
```

## 9. 风险评估和缓解策略

### 9.1 性能风险

- **大数据量处理**：使用分批处理和多线程
- **内存占用**：实现内存池和智能缓存清理
- **实时性要求**：优化算法复杂度，使用增量更新

### 9.2 算法准确性风险

- **误判连拍**：多维度验证机制
- **漏判连拍**：可调节参数配置
- **特殊场景**：充分的测试覆盖

### 9.3 平台兼容性风险

- **不同设备差异**：抽象平台特定逻辑
- **React Native 版本兼容**：使用稳定的 TurboModule API
- **内存模型差异**：统一的内存管理策略

## 10. 监控和指标

### 10.1 性能指标

- 处理时间（每 1000 张照片）
- 内存使用峰值
- 缓存命中率
- API 响应时间

### 10.2 质量指标

- 连拍识别准确率
- 误判率
- 漏判率
- 用户满意度

这个技术设计方案提供了完整的照片连拍识别功能实现路径，兼顾了性能、准确性和可维护性。请 review 此方案，如果有任何调整建议，我可以进一步完善。
