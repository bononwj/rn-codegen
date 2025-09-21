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

export interface IncrementalUpdateResult {
  newGroups: BurstGroup[];
  modifiedGroupIds: string[];
  deletedGroupIds: string[];
  processingTimeMs: number;
}

export interface PerformanceStats {
  totalPhotos: number;
  totalGroups: number;
  cacheHitRate: number;
  lastProcessingTimeMs: number;
}

export interface Spec extends TurboModule {
  // 初始化并分析所有照片
  readonly detectBursts: (
    photos: PhotoInfo[],
    config?: BurstDetectionConfig,
  ) => Promise<BurstDetectionResult>;

  // 增量添加新照片
  readonly addPhotos: (photos: PhotoInfo[]) => Promise<IncrementalUpdateResult>;

  // 移除照片
  readonly removePhotos: (photoIds: string[]) => Promise<IncrementalUpdateResult>;

  // 更新照片信息
  readonly updatePhotos: (photos: PhotoInfo[]) => Promise<IncrementalUpdateResult>;

  // 批量更新操作
  readonly batchUpdate: (
    addedPhotos: PhotoInfo[],
    removedPhotoIds: string[],
    updatedPhotos: PhotoInfo[],
  ) => Promise<IncrementalUpdateResult>;

  // 获取指定分组的详细信息
  readonly getGroupDetails: (groupId: string) => Promise<BurstGroup | null>;

  // 获取所有分组的简要信息（用于列表展示）
  readonly getAllGroups: () => Promise<BurstGroup[]>;

  // 获取照片所属分组
  readonly getPhotoGroup: (photoId: string) => Promise<string | null>;

  // 获取分组内的所有照片
  readonly getPhotosInGroup: (groupId: string) => Promise<string[]>;

  // 分组管理操作
  readonly mergeGroups: (
    groupIds: string[],
    newGroupId?: string,
  ) => Promise<string>;

  readonly splitGroup: (
    groupId: string,
    photoGroups: string[][],
  ) => Promise<string[]>;

  // 清除所有数据和缓存
  readonly clearAll: () => Promise<void>;

  // 获取性能统计信息
  readonly getPerformanceStats: () => Promise<PerformanceStats>;

  // 配置管理
  readonly updateConfig: (config: BurstDetectionConfig) => Promise<void>;

  readonly getConfig: () => Promise<BurstDetectionConfig>;

  // 缓存管理
  readonly clearCache: () => Promise<void>;

  readonly getCacheStats: () => Promise<{
    totalCacheSize: number;
    hitRate: number;
    groupCacheSize: number;
    timestampCacheSize: number;
    resultCacheSize: number;
  }>;

  // 验证和维护
  readonly validateConsistency: () => Promise<boolean>;

  readonly removeEmptyGroups: () => Promise<string[]>;

  // 开发和调试接口
  readonly generateTestData: (
    scenario: string,
    photoCount?: number,
  ) => Promise<PhotoInfo[]>;

  readonly exportGroupsToJson: () => Promise<string>;

  readonly importGroupsFromJson: (jsonData: string) => Promise<void>;
}

export default TurboModuleRegistry.getEnforcing<Spec>(
  'NativePhotoBurstDetection',
);
