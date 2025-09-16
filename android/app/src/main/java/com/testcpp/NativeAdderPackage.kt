package com.testcpp

import com.facebook.react.TurboReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfo
import com.facebook.react.module.model.ReactModuleInfoProvider

class NativeAdderPackage : TurboReactPackage() {
  override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? =
    if (name == "NativeAdder") NativeAdderModule(reactContext) else null

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider = ReactModuleInfoProvider {
    mapOf(
      "NativeAdder" to ReactModuleInfo(
        "NativeAdder",
        "NativeAdderModule",
        false, // canUseCPT
        false, // needsEagerInit
        false, // hasConstants
        false, // isCxxModule
        true // isTurboModule
      )
    )
  }
}
