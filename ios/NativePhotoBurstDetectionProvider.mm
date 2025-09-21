#import "NativePhotoBurstDetectionProvider.h"
#import "../src/shared/burst-detection/NativePhotoBurstDetection.h"

@implementation NativePhotoBurstDetectionProvider

- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:(const std::string &)name
                                                      jsInvoker:(std::shared_ptr<facebook::react::CallInvoker>)jsInvoker {
    if (name == "NativePhotoBurstDetection") {
        return std::make_shared<facebook::react::NativePhotoBurstDetection>(jsInvoker);
    }
    return nullptr;
}

@end
