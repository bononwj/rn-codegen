import React, { useState, useEffect } from 'react';
import {
  StyleSheet,
  Text,
  View,
  ScrollView,
  TouchableOpacity,
  Alert,
  ActivityIndicator,
} from 'react-native';

import NativePhotoBurstDetection, {
  PhotoInfo,
  BurstGroup,
  BurstDetectionResult,
  BurstDetectionConfig,
  PerformanceStats,
} from './src/specs/NativePhotoBurstDetection';

interface TestScenario {
  id: string;
  name: string;
  description: string;
  photoCount: number;
}

const App: React.FC = () => {
  const [isLoading, setIsLoading] = useState(false);
  const [detectionResult, setDetectionResult] =
    useState<BurstDetectionResult | null>(null);
  const [performanceStats, setPerformanceStats] =
    useState<PerformanceStats | null>(null);
  const [selectedScenario, setSelectedScenario] =
    useState<string>('basic_burst');

  const testScenarios: TestScenario[] = [
    {
      id: 'basic_burst',
      name: '基础连拍测试',
      description: '5张连续拍摄的照片',
      photoCount: 5,
    },
    {
      id: 'mixed_photos',
      name: '混合场景测试',
      description: '包含连拍、单张照片的混合场景',
      photoCount: 12,
    },
    {
      id: 'performance_test',
      name: '性能测试',
      description: '大量照片的性能测试',
      photoCount: 1000,
    },
  ];

  const runBurstDetection = async (
    scenario: string,
    photoCount: number = 100,
  ) => {
    setIsLoading(true);
    try {
      console.log(`开始运行场景: ${scenario}`);

      // 1. 生成测试数据
      const testPhotos = await NativePhotoBurstDetection.generateTestData(
        scenario,
        photoCount,
      );
      console.log(`生成了 ${testPhotos.length} 张测试照片`);

      // 2. 配置检测参数
      const config: BurstDetectionConfig = {
        maxTimeGapMs: 2000,
        minBurstSize: 3,
        enableGpsAnalysis: false,
        enableSimilarityCheck: false,
      };

      // 3. 执行连拍检测
      const startTime = Date.now();
      const result = await NativePhotoBurstDetection.detectBursts(
        testPhotos,
        config,
      );
      const endTime = Date.now();

      console.log(`检测完成，耗时: ${endTime - startTime}ms`);
      console.log(`检测结果:`, result);

      setDetectionResult(result);

      // 4. 获取性能统计
      const stats = await NativePhotoBurstDetection.getPerformanceStats();
      setPerformanceStats(stats);

      // 5. 显示结果
      Alert.alert(
        '检测完成',
        `发现 ${result.burstGroupCount} 个连拍分组，${result.singlePhotoCount} 张单独照片\n处理时间: ${result.processingTimeMs}ms`,
      );
    } catch (error) {
      console.error('连拍检测失败:', error);
      Alert.alert('错误', `检测失败: ${error}`);
    } finally {
      setIsLoading(false);
    }
  };

  const testIncrementalUpdate = async () => {
    setIsLoading(true);
    try {
      console.log('开始增量更新测试');

      // 1. 生成初始数据
      const initialPhotos = await NativePhotoBurstDetection.generateTestData(
        'basic_burst',
        10,
      );
      const initialResult = await NativePhotoBurstDetection.detectBursts(
        initialPhotos,
      );
      console.log('初始检测结果:', initialResult);

      // 2. 添加新照片
      const newPhotos = await NativePhotoBurstDetection.generateTestData(
        'basic_burst',
        5,
      );
      const addResult = await NativePhotoBurstDetection.addPhotos(newPhotos);
      console.log('添加照片结果:', addResult);

      // 3. 获取所有分组
      const allGroups = await NativePhotoBurstDetection.getAllGroups();
      console.log('所有分组:', allGroups);

      Alert.alert(
        '增量更新测试完成',
        `新增 ${addResult.newGroups.length} 个分组\n修改 ${addResult.modifiedGroupIds.length} 个分组\n处理时间: ${addResult.processingTimeMs}ms`,
      );
    } catch (error) {
      console.error('增量更新测试失败:', error);
      Alert.alert('错误', `增量更新失败: ${error}`);
    } finally {
      setIsLoading(false);
    }
  };

  const clearAllData = async () => {
    try {
      await NativePhotoBurstDetection.clearAll();
      setDetectionResult(null);
      setPerformanceStats(null);
      Alert.alert('成功', '已清除所有数据');
    } catch (error) {
      Alert.alert('错误', `清除数据失败: ${error}`);
    }
  };

  const validateConsistency = async () => {
    try {
      const isValid = await NativePhotoBurstDetection.validateConsistency();
      Alert.alert(
        '数据一致性检查',
        isValid ? '数据一致性良好' : '发现数据不一致',
      );
    } catch (error) {
      Alert.alert('错误', `一致性检查失败: ${error}`);
    }
  };

  const renderDetectionResult = () => {
    if (!detectionResult) return null;

    return (
      <View style={styles.resultContainer}>
        <Text style={styles.resultTitle}>检测结果</Text>
        <Text style={styles.resultText}>
          总照片数: {detectionResult.totalPhotos}
        </Text>
        <Text style={styles.resultText}>
          连拍分组: {detectionResult.burstGroupCount}
        </Text>
        <Text style={styles.resultText}>
          单张照片: {detectionResult.singlePhotoCount}
        </Text>
        <Text style={styles.resultText}>
          处理耗时: {detectionResult.processingTimeMs}ms
        </Text>

        <Text style={styles.groupsTitle}>分组详情:</Text>
        {detectionResult.groups.map((group, index) => (
          <View key={group.groupId} style={styles.groupItem}>
            <Text style={styles.groupText}>
              分组 {index + 1}: {group.type} ({group.photoCount} 张)
            </Text>
            <Text style={styles.groupSubText}>
              时间范围: {new Date(group.startTime).toLocaleTimeString()} -{' '}
              {new Date(group.endTime).toLocaleTimeString()}
            </Text>
          </View>
        ))}
      </View>
    );
  };

  const renderPerformanceStats = () => {
    if (!performanceStats) return null;

    return (
      <View style={styles.statsContainer}>
        <Text style={styles.statsTitle}>性能统计</Text>
        <Text style={styles.statsText}>
          总照片数: {performanceStats.totalPhotos}
        </Text>
        <Text style={styles.statsText}>
          总分组数: {performanceStats.totalGroups}
        </Text>
        <Text style={styles.statsText}>
          缓存命中率: {(performanceStats.cacheHitRate * 100).toFixed(2)}%
        </Text>
        <Text style={styles.statsText}>
          最后处理时间: {performanceStats.lastProcessingTimeMs}ms
        </Text>
      </View>
    );
  };

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>照片连拍识别测试</Text>

      {/* 测试场景选择 */}
      <View style={styles.scenarioContainer}>
        <Text style={styles.sectionTitle}>测试场景</Text>
        {testScenarios.map(scenario => (
          <TouchableOpacity
            key={scenario.id}
            style={[
              styles.scenarioButton,
              selectedScenario === scenario.id && styles.selectedScenario,
            ]}
            onPress={() => setSelectedScenario(scenario.id)}
          >
            <Text style={styles.scenarioName}>{scenario.name}</Text>
            <Text style={styles.scenarioDescription}>
              {scenario.description}
            </Text>
            <Text style={styles.scenarioPhotoCount}>
              {scenario.photoCount} 张照片
            </Text>
          </TouchableOpacity>
        ))}
      </View>

      {/* 操作按钮 */}
      <View style={styles.buttonContainer}>
        <TouchableOpacity
          style={styles.button}
          onPress={() => {
            const scenario = testScenarios.find(s => s.id === selectedScenario);
            runBurstDetection(selectedScenario, scenario?.photoCount);
          }}
          disabled={isLoading}
        >
          <Text style={styles.buttonText}>运行连拍检测</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={styles.button}
          onPress={testIncrementalUpdate}
          disabled={isLoading}
        >
          <Text style={styles.buttonText}>测试增量更新</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={styles.button}
          onPress={validateConsistency}
          disabled={isLoading}
        >
          <Text style={styles.buttonText}>验证数据一致性</Text>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.button, styles.clearButton]}
          onPress={clearAllData}
          disabled={isLoading}
        >
          <Text style={styles.buttonText}>清除所有数据</Text>
        </TouchableOpacity>
      </View>

      {/* 加载指示器 */}
      {isLoading && (
        <View style={styles.loadingContainer}>
          <ActivityIndicator size="large" color="#007AFF" />
          <Text style={styles.loadingText}>处理中...</Text>
        </View>
      )}

      {/* 检测结果 */}
      {renderDetectionResult()}

      {/* 性能统计 */}
      {renderPerformanceStats()}
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 20,
    color: '#333',
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 10,
    color: '#333',
  },
  scenarioContainer: {
    marginBottom: 20,
  },
  scenarioButton: {
    backgroundColor: '#fff',
    padding: 15,
    marginBottom: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#ddd',
  },
  selectedScenario: {
    borderColor: '#007AFF',
    backgroundColor: '#f0f8ff',
  },
  scenarioName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
  },
  scenarioDescription: {
    fontSize: 14,
    color: '#666',
    marginTop: 2,
  },
  scenarioPhotoCount: {
    fontSize: 12,
    color: '#999',
    marginTop: 4,
  },
  buttonContainer: {
    marginBottom: 20,
  },
  button: {
    backgroundColor: '#007AFF',
    padding: 15,
    borderRadius: 8,
    marginBottom: 10,
    alignItems: 'center',
  },
  clearButton: {
    backgroundColor: '#FF3B30',
  },
  buttonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  loadingContainer: {
    alignItems: 'center',
    padding: 20,
  },
  loadingText: {
    marginTop: 10,
    fontSize: 16,
    color: '#666',
  },
  resultContainer: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
  },
  resultTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 10,
    color: '#333',
  },
  resultText: {
    fontSize: 14,
    marginBottom: 5,
    color: '#666',
  },
  groupsTitle: {
    fontSize: 16,
    fontWeight: '600',
    marginTop: 10,
    marginBottom: 5,
    color: '#333',
  },
  groupItem: {
    backgroundColor: '#f8f8f8',
    padding: 10,
    borderRadius: 5,
    marginBottom: 5,
  },
  groupText: {
    fontSize: 14,
    fontWeight: '500',
    color: '#333',
  },
  groupSubText: {
    fontSize: 12,
    color: '#666',
    marginTop: 2,
  },
  statsContainer: {
    backgroundColor: '#fff',
    padding: 15,
    borderRadius: 8,
    marginBottom: 20,
  },
  statsTitle: {
    fontSize: 18,
    fontWeight: '600',
    marginBottom: 10,
    color: '#333',
  },
  statsText: {
    fontSize: 14,
    marginBottom: 5,
    color: '#666',
  },
});

export default App;
