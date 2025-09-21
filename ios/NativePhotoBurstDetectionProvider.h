#pragma once

#include <ReactCommon/TurboModule.h>
#include <memory>

namespace facebook::react {

class NativePhotoBurstDetection;

class NativePhotoBurstDetectionProvider {
public:
  static std::shared_ptr<TurboModule>
  getTurboModule(const std::string &name,
                 std::shared_ptr<CallInvoker> jsInvoker);

private:
  static std::shared_ptr<NativePhotoBurstDetection> burstDetectionModule_;
};

} // namespace facebook::react
