package com.testcpp

import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.annotations.ReactModule

@ReactModule(name = NativePhotoBurstDetectionModule.NAME)
class NativePhotoBurstDetectionModule(reactContext: ReactApplicationContext) :
    NativePhotoBurstDetectionSpec(reactContext) {

    override fun getName(): String {
        return NAME
    }

    companion object {
        const val NAME = "NativePhotoBurstDetection"
    }

    // TurboModules load synchronously. This means that they have immediate access to ReactApplicationContext
    // and can be used right after instantiation.
    // For this reason we should avoid doing heavy work in the constructor.
    
    init {
        // Initialize native C++ module if needed
    }
}
