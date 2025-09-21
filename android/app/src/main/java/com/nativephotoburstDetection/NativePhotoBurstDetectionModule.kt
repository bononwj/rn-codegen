package com.nativephotoburstDetection

import android.content.Context
import com.willapp.specs.NativePhotoBurstDetectionSpec
import com.facebook.react.bridge.ReactApplicationContext

class NativePhotoBurstDetectionModule(reactContext: ReactApplicationContext) : NativePhotoBurstDetectionSpec(reactContext) {

  override fun getName() = NAME
    companion object {
        const val NAME = "NativePhotoBurstDetection"
    }

    // TurboModules load synchronously. This means that they have immediate access to ReactApplicationContext
    // and can be used right after instantiation.
    // For this reason we should avoid doing heavy work in the constructor.
    
    init {
        // Initialize native C++ module if needed
    }

    override fun detectBursts(photos: List<PhotoInfo>, config: BurstDetectionConfig): BurstDetectionResult {
        return super.detectBursts(photos, config)
    }

    override fun addPhotos(photos: List<PhotoInfo>): List<PhotoInfo> {
        return super.addPhotos(photos)
    }

    override fun removePhotos(photoIds: List<String>): List<PhotoInfo> {
        return super.removePhotos(photoIds)
    }
    
    override fun updatePhotos(photos: List<PhotoInfo>): List<PhotoInfo> {
        return super.updatePhotos(photos)
    }

    override fun getGroupDetails(groupId: String): BurstGroup {
        return super.getGroupDetails(groupId)
    }
    
    override fun getAllGroups(): List<BurstGroup> {
        return super.getAllGroups()
    }

    override fun getPhotoGroup(photoId: String): String {
        return super.getPhotoGroup(photoId)
    }
    
    override fun getPhotosInGroup(groupId: String): List<PhotoInfo> {
        return super.getPhotosInGroup(groupId)
    }

    override fun mergeGroups(groupIds: List<String>, newGroupId: String): String {
        return super.mergeGroups(groupIds, newGroupId)
    }
    
    override fun splitGroup(groupId: String, photoGroups: List<List<String>>): List<String> {
        return super.splitGroup(groupId, photoGroups)
    }

    override fun clearAll(): Unit {
        return super.clearAll()
    }
    
    override fun getPerformanceStats(): PerformanceStats {
        return super.getPerformanceStats()
    }

    override fun updateConfig(config: BurstDetectionConfig): Unit {
        return super.updateConfig(config)
    }

    override fun getConfig(): BurstDetectionConfig {
        return super.getConfig()
    }

    override fun clearCache(): Unit {
        return super.clearCache()
    }

    override fun getCacheStats(): CacheStats {
        return super.getCacheStats()
    }

    override fun validateConsistency(): Boolean {
        return super.validateConsistency()
    }
    
    override fun removeEmptyGroups(): List<String> {
        return super.removeEmptyGroups()
    }

    override fun generateTestData(scenario: String, photoCount: Int): List<PhotoInfo> {
        return super.generateTestData(scenario, photoCount)
    }
    
    override fun exportGroupsToJson(): String {
        return super.exportGroupsToJson()
    }

    override fun importGroupsFromJson(jsonData: String): Unit {
        return super.importGroupsFromJson(jsonData)
    }
    
    override fun batchUpdate(addedPhotos: List<PhotoInfo>, removedPhotoIds: List<String>, updatedPhotos: List<PhotoInfo>): List<PhotoInfo> {
        return super.batchUpdate(addedPhotos, removedPhotoIds, updatedPhotos)
    }
    
}
