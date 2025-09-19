package com.webview

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider
import com.facebook.react.uimanager.ViewManager

class ReactWebViewPackage : BaseReactPackage() {
    override fun createViewManagers(reactContext: ReactApplicationContext): List<ViewManager<*, *>> {
        return listOf(ReactWebViewManager(reactContext))
    }

    override fun getModule(s: String, reactApplicationContext: ReactApplicationContext): NativeModule? {
        when (s) {
            ReactWebViewManager.REACT_CLASS -> ReactWebViewManager(reactApplicationContext)
        }
        return null
    }

    override fun getReactModuleInfoProvider(): ReactModuleInfoProvider = ReactModuleInfoProvider {
        mapOf(ReactWebViewManager.REACT_CLASS to ReactModuleInfo(
            name = ReactWebViewManager.REACT_CLASS,
            className = ReactWebViewManager.REACT_CLASS,
            canOverrideExistingModule = false,
            needsEagerInit = false,
            isCxxModule = false,
            isTurboModule = true,
        )
        )
    }
}