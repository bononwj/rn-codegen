#import "RCTWebView.h"

#import <react/renderer/components/AppSpecs/ComponentDescriptors.h>
#import <react/renderer/components/AppSpecs/EventEmitters.h>
#import <react/renderer/components/AppSpecs/Props.h>
#import <react/renderer/components/AppSpecs/RCTComponentViewHelpers.h>
#import <WebKit/WebKit.h>

using namespace facebook::react;

@interface RCTWebView () <RCTCustomWebViewViewProtocol, WKNavigationDelegate>
@end

@implementation RCTWebView {
  NSURL * _sourceURL;
  WKWebView * _webView;
}

-(instancetype)init
{
  if (![NSThread isMainThread]) {
    __block RCTWebView *initializedSelf = nil;
    dispatch_sync(dispatch_get_main_queue(), ^{
      initializedSelf = [super init];
      if (initializedSelf) {
        initializedSelf->_webView = [WKWebView new];
        initializedSelf->_webView.navigationDelegate = initializedSelf;
        [initializedSelf addSubview:initializedSelf->_webView];
        if (initializedSelf->_sourceURL) {
          [initializedSelf->_webView loadRequest:[NSURLRequest requestWithURL:initializedSelf->_sourceURL]];
        }
      }
    });
    return initializedSelf;
  }

  self = [super init];
  if (self) {
    _webView = [WKWebView new];
    _webView.navigationDelegate = self;
    [self addSubview:_webView];
  }
  return self;
}

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  const auto &oldViewProps = *std::static_pointer_cast<CustomWebViewProps const>(_props);
  const auto &newViewProps = *std::static_pointer_cast<CustomWebViewProps const>(props);

  // Handle your props here
  if (oldViewProps.sourceURL != newViewProps.sourceURL) {
    NSString *urlString = [NSString stringWithCString:newViewProps.sourceURL.c_str() encoding:NSUTF8StringEncoding];
    _sourceURL = [NSURL URLWithString:urlString];
    if ([self urlIsValid:newViewProps.sourceURL]) {
      [_webView loadRequest:[NSURLRequest requestWithURL:_sourceURL]];
    }
  }

  [super updateProps:props oldProps:oldProps];
}

-(void)layoutSubviews
{
  [super layoutSubviews];
  _webView.frame = self.bounds;

}

#pragma mark - WKNavigationDelegate

-(void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
  CustomWebViewEventEmitter::OnScriptLoaded result = CustomWebViewEventEmitter::OnScriptLoaded{CustomWebViewEventEmitter::OnScriptLoadedResult::Success};
  self.eventEmitter.onScriptLoaded(result);
}

- (BOOL)urlIsValid:(std::string)propString
{
  if (propString.length() > 0 && !_sourceURL) {
    CustomWebViewEventEmitter::OnScriptLoaded result = CustomWebViewEventEmitter::OnScriptLoaded{CustomWebViewEventEmitter::OnScriptLoadedResult::Error};

    self.eventEmitter.onScriptLoaded(result);
    return NO;
  }
  return YES;
}

// Event emitter convenience method
- (const CustomWebViewEventEmitter &)eventEmitter
{
  return static_cast<const CustomWebViewEventEmitter &>(*_eventEmitter);
}

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
  return concreteComponentDescriptorProvider<CustomWebViewComponentDescriptor>();
}

@end
