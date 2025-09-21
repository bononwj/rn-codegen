#include "PhotoGroupManager.h"
#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>

namespace facebook::react {

PhotoGroupManager::PhotoGroupManager() = default;

PhotoGroupManager::~PhotoGroupManager() = default;

void PhotoGroupManager::addGroup(const BurstGroup &group) {
  groups_[group.groupId] = group;
  updatePhotoMapping(group);
}

void PhotoGroupManager::updateGroup(const BurstGroup &group) {
  auto it = groups_.find(group.groupId);
  if (it != groups_.end()) {
    // 先移除旧的照片映射
    removePhotoMapping(group.groupId);
    // 更新分组信息
    it->second = group;
    // 重新建立照片映射
    updatePhotoMapping(group);
  }
}

void PhotoGroupManager::removeGroup(const std::string &groupId) {
  auto it = groups_.find(groupId);
  if (it != groups_.end()) {
    removePhotoMapping(groupId);
    groups_.erase(it);
  }
}

void PhotoGroupManager::clearAllGroups() {
  groups_.clear();
  photoToGroup_.clear();
  groupToPhotos_.clear();
}

BurstGroup *PhotoGroupManager::getGroup(const std::string &groupId) {
  auto it = groups_.find(groupId);
  return it != groups_.end() ? &it->second : nullptr;
}

const BurstGroup *
PhotoGroupManager::getGroup(const std::string &groupId) const {
  auto it = groups_.find(groupId);
  return it != groups_.end() ? &it->second : nullptr;
}

std::vector<BurstGroup> PhotoGroupManager::getAllGroups() const {
  std::vector<BurstGroup> groups;
  groups.reserve(groups_.size());
  for (const auto &pair : groups_) {
    groups.push_back(pair.second);
  }

  // 按时间排序
  std::sort(groups.begin(), groups.end(),
            [](const BurstGroup &a, const BurstGroup &b) {
              return a.startTime < b.startTime;
            });

  return groups;
}

std::vector<std::string> PhotoGroupManager::getAllGroupIds() const {
  std::vector<std::string> groupIds;
  groupIds.reserve(groups_.size());
  for (const auto &pair : groups_) {
    groupIds.push_back(pair.first);
  }
  return groupIds;
}

std::string PhotoGroupManager::getPhotoGroup(const std::string &photoId) const {
  auto it = photoToGroup_.find(photoId);
  return it != photoToGroup_.end() ? it->second : "";
}

std::vector<std::string>
PhotoGroupManager::getPhotosInGroup(const std::string &groupId) const {
  auto it = groupToPhotos_.find(groupId);
  if (it != groupToPhotos_.end()) {
    return std::vector<std::string>(it->second.begin(), it->second.end());
  }
  return {};
}

bool PhotoGroupManager::isPhotoInGroup(const std::string &photoId,
                                       const std::string &groupId) const {
  auto it = groupToPhotos_.find(groupId);
  if (it != groupToPhotos_.end()) {
    return it->second.find(photoId) != it->second.end();
  }
  return false;
}

void PhotoGroupManager::addPhotoToGroup(const std::string &photoId,
                                        const std::string &groupId) {
  // 如果照片已经在其他分组中，先移除
  auto currentGroupId = getPhotoGroup(photoId);
  if (!currentGroupId.empty() && currentGroupId != groupId) {
    removePhotoFromGroup(photoId, currentGroupId);
  }

  // 添加到新分组
  photoToGroup_[photoId] = groupId;
  groupToPhotos_[groupId].insert(photoId);

  // 更新分组信息
  auto group = getGroup(groupId);
  if (group) {
    auto &photoIds = group->photoIds;
    if (std::find(photoIds.begin(), photoIds.end(), photoId) ==
        photoIds.end()) {
      photoIds.push_back(photoId);
      updateGroupStatistics(*group);
    }
  }
}

void PhotoGroupManager::removePhotoFromGroup(const std::string &photoId,
                                             const std::string &groupId) {
  // 移除照片映射
  photoToGroup_.erase(photoId);

  auto it = groupToPhotos_.find(groupId);
  if (it != groupToPhotos_.end()) {
    it->second.erase(photoId);
  }

  // 更新分组信息
  auto group = getGroup(groupId);
  if (group) {
    auto &photoIds = group->photoIds;
    photoIds.erase(std::remove(photoIds.begin(), photoIds.end(), photoId),
                   photoIds.end());
    updateGroupStatistics(*group);
  }
}

void PhotoGroupManager::movePhotoToGroup(const std::string &photoId,
                                         const std::string &fromGroupId,
                                         const std::string &toGroupId) {
  removePhotoFromGroup(photoId, fromGroupId);
  addPhotoToGroup(photoId, toGroupId);
}

std::string
PhotoGroupManager::mergeGroups(const std::vector<std::string> &groupIds,
                               const std::string &newGroupId) {
  if (groupIds.size() < 2) {
    return "";
  }

  // 创建新分组ID
  std::string mergedGroupId =
      newGroupId.empty() ? generateUniqueGroupId() : newGroupId;

  // 收集所有照片
  std::vector<std::string> allPhotos;
  int64_t minStartTime = std::numeric_limits<int64_t>::max();
  int64_t maxEndTime = 0;
  GroupType mergedType = GroupType::SINGLE;

  for (const std::string &groupId : groupIds) {
    auto group = getGroup(groupId);
    if (group) {
      allPhotos.insert(allPhotos.end(), group->photoIds.begin(),
                       group->photoIds.end());
      minStartTime = std::min(minStartTime, group->startTime);
      maxEndTime = std::max(maxEndTime, group->endTime);

      // 如果任何一个是连拍，合并后也是连拍
      if (group->type == GroupType::BURST) {
        mergedType = GroupType::BURST;
      }
    }
  }

  // 创建合并后的分组
  BurstGroup mergedGroup;
  mergedGroup.groupId = mergedGroupId;
  mergedGroup.photoIds = allPhotos;
  mergedGroup.startTime = minStartTime;
  mergedGroup.endTime = maxEndTime;
  mergedGroup.type = mergedType;
  updateGroupStatistics(mergedGroup);

  // 添加新分组
  addGroup(mergedGroup);

  // 移除旧分组
  for (const std::string &groupId : groupIds) {
    removeGroup(groupId);
  }

  return mergedGroupId;
}

std::vector<std::string> PhotoGroupManager::splitGroup(
    const std::string &groupId,
    const std::vector<std::vector<std::string>> &photoGroups) {
  auto originalGroup = getGroup(groupId);
  if (!originalGroup) {
    return {};
  }

  std::vector<std::string> newGroupIds;

  // 为每个照片子组创建新分组
  for (const auto &photos : photoGroups) {
    if (photos.empty()) {
      continue;
    }

    std::string newGroupId = generateUniqueGroupId();
    BurstGroup newGroup;
    newGroup.groupId = newGroupId;
    newGroup.photoIds = photos;
    newGroup.type = photos.size() >= 3 ? GroupType::BURST : GroupType::SINGLE;
    updateGroupStatistics(newGroup);

    addGroup(newGroup);
    newGroupIds.push_back(newGroupId);
  }

  // 移除原分组
  removeGroup(groupId);

  return newGroupIds;
}

size_t PhotoGroupManager::getGroupCount() const { return groups_.size(); }

size_t PhotoGroupManager::getPhotoCount() const { return photoToGroup_.size(); }

size_t PhotoGroupManager::getBurstGroupCount() const {
  size_t count = 0;
  for (const auto &pair : groups_) {
    if (pair.second.type == GroupType::BURST) {
      count++;
    }
  }
  return count;
}

size_t PhotoGroupManager::getSinglePhotoCount() const {
  size_t count = 0;
  for (const auto &pair : groups_) {
    if (pair.second.type == GroupType::SINGLE) {
      count++;
    }
  }
  return count;
}

void PhotoGroupManager::validateConsistency() {
  // 验证照片映射的一致性
  std::unordered_set<std::string> allPhotos;

  // 收集所有分组中的照片
  for (const auto &groupPair : groups_) {
    const auto &group = groupPair.second;
    for (const std::string &photoId : group.photoIds) {
      if (allPhotos.find(photoId) != allPhotos.end()) {
        // 发现重复照片，需要修复
        // 这里可以实现修复逻辑
      }
      allPhotos.insert(photoId);
    }
  }

  // 验证反向映射
  for (const auto &photoPair : photoToGroup_) {
    const std::string &photoId = photoPair.first;
    const std::string &groupId = photoPair.second;

    auto group = getGroup(groupId);
    if (!group) {
      // 分组不存在，移除映射
      photoToGroup_.erase(photoId);
      continue;
    }

    auto &photoIds = group->photoIds;
    if (std::find(photoIds.begin(), photoIds.end(), photoId) ==
        photoIds.end()) {
      // 分组中没有这个照片，修复映射
      photoIds.push_back(photoId);
    }
  }
}

void PhotoGroupManager::removeEmptyGroups() {
  std::vector<std::string> emptyGroupIds;

  for (const auto &pair : groups_) {
    if (pair.second.photoIds.empty()) {
      emptyGroupIds.push_back(pair.first);
    }
  }

  for (const std::string &groupId : emptyGroupIds) {
    removeGroup(groupId);
  }
}

void PhotoGroupManager::updatePhotoMapping(const BurstGroup &group) {
  for (const std::string &photoId : group.photoIds) {
    photoToGroup_[photoId] = group.groupId;
    groupToPhotos_[group.groupId].insert(photoId);
  }
}

void PhotoGroupManager::removePhotoMapping(const std::string &groupId) {
  auto it = groupToPhotos_.find(groupId);
  if (it != groupToPhotos_.end()) {
    // 移除所有照片的反向映射
    for (const std::string &photoId : it->second) {
      photoToGroup_.erase(photoId);
    }
    groupToPhotos_.erase(it);
  }
}

std::string PhotoGroupManager::generateUniqueGroupId() const {
  auto now = std::chrono::high_resolution_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                       now.time_since_epoch())
                       .count();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);

  std::stringstream ss;
  ss << "group_" << timestamp << "_" << dis(gen);
  return ss.str();
}

void PhotoGroupManager::updateGroupStatistics(BurstGroup &group) {
  group.photoCount = static_cast<int32_t>(group.photoIds.size());

  // 选择代表照片（中间位置）
  if (!group.photoIds.empty()) {
    size_t middleIndex = group.photoIds.size() / 2;
    group.representativePhotoId = group.photoIds[middleIndex];
  }

  // 更新分组类型
  if (group.photoCount >= 3) {
    group.type = GroupType::BURST;
  } else {
    group.type = GroupType::SINGLE;
  }
}

} // namespace facebook::react
