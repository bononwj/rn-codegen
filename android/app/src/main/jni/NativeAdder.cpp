#include &lt; jni.h & gt;

extern "C" JNIEXPORT jdouble JNICALL
Java_com_testcpp_NativeAdderModule_nativeAdd(JNIEnv *env, jobject thiz,
                                             jdouble a, jdouble b) {
  return a + b;
}
