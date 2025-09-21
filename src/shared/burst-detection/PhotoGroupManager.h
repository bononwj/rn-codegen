#pragma once

#include "Types.h"
#include <unordered_map>
#include <unordered_set>

namespace facebook::react {

class PhotoGroupManager {
public:
  PhotoGroupManager();
  ~PhotoGroupManager();

  // 分组管理操作
  void addGroup(const BurstGroup &group);
  void updateGroup(const BurstGroup &group);
  void removeGroup(const std::string &groupId);
  void clearAllGroups();

  // 查询操作
  BurstGroup *getGroup(const std::string &groupId);
  const BurstGroup *getGroup(const std::string &groupId) const;
  std::vector<BurstGroup> getAllGroups() const;
  std::vector<std::string> getAllGroupIds() const;

  // 照片关联查询
  std::string getPhotoGroup(const std::string &photoId) const;
  std::vector<std::string> getPhotosInGroup(const std::string &groupId) const;
  bool isPhotoInGroup(const std::string &photoId,
                      const std::string &groupId) const;

  // 照片操作
  void addPhotoToGroup(const std::string &photoId, const std::string &groupId);
  void removePhotoFromGroup(const std::string &photoId,
                            const std::string &groupId);
  void movePhotoToGroup(const std::string &photoId,
                        const std::string &fromGroupId,
                        const std::string &toGroupId);

  // 分组合并和拆分
  std::string mergeGroups(const std::vector<std::string> &groupIds,
                          const std::string &newGroupId = "");
  std::vector<std::string>
  splitGroup(const std::string &groupId,
             const std::vector<std::vector<std::string>> &photoGroups);

  // 统计信息
  size_t getGroupCount() const;
  size_t getPhotoCount() const;
  size_t getBurstGroupCount() const;
  size_t getSinglePhotoCount() const;

  // 验证和清理
  void validateConsistency();
  void removeEmptyGroups();

private:
  // 存储结构
  std::unordered_map<std::string, BurstGroup> groups_; // 分组ID -> 分组信息
  std::unordered_map<std::string, std::string>
      photoToGroup_; // 照片ID -> 分组ID
  std::unordered_map<std::string, std::unordered_set<std::string>>
      groupToPhotos_; // 分组ID -> 照片ID集合

  // 内部工具方法
  void updatePhotoMapping(const BurstGroup &group);
  void removePhotoMapping(const std::string &groupId);
  std::string generateUniqueGroupId() const;
  void updateGroupStatistics(BurstGroup &group);
};

} // namespace facebook::react
