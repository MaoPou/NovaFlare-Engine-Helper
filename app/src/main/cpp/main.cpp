#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>
#include <string>
#include <thread>

#define LOG_TAG "NovaFlareHelper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 启动 NovaFlareEngine 的函数
void launchNovaFlareEngine(ANativeActivity* activity) {
    LOGI("Attempting to launch NovaFlareEngine...");
    
    JNIEnv* env = activity->env;
    JavaVM* vm = activity->vm;
    
    // 附加当前线程到 JVM
    vm->AttachCurrentThread(&env, nullptr);
    
    // 获取 Context 类
    jclass contextClass = env->GetObjectClass(activity->clazz);
    
    // 获取 getPackageManager 方法
    jmethodID getPackageManager = env->GetMethodID(
        contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    
    // 调用 getPackageManager
    jobject packageManager = env->CallObjectMethod(activity->clazz, getPackageManager);
    
    // 获取 PackageManager 类
    jclass pmClass = env->FindClass("android/content/pm/PackageManager");
    
    // 获取 getLaunchIntentForPackage 方法
    jmethodID getLaunchIntent = env->GetMethodID(
        pmClass, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
    
    // 创建包名字符串
    jstring packageName = env->NewStringUTF("com.NovaFlareEngine");
    
    // 获取启动 Intent
    jobject intent = env->CallObjectMethod(packageManager, getLaunchIntent, packageName);
    
    if (intent != nullptr) {
        LOGI("Found NovaFlareEngine, starting activity...");
        
        // 获取 Context 类中的 startActivity 方法
        jmethodID startActivity = env->GetMethodID(
            contextClass, "startActivity", "(Landroid/content/Intent;)V");
        
        // 启动活动
        env->CallVoidMethod(activity->clazz, startActivity, intent);
        LOGI("NovaFlareEngine launched successfully!");
    } else {
        LOGE("NovaFlareEngine not found or not installed!");
        
        // 显示错误消息
        jclass toastClass = env->FindClass("android/widget/Toast");
        jmethodID makeText = env->GetStaticMethodID(
            toastClass, "makeText", 
            "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
        
        jstring message = env->NewStringUTF("NovaFlareEngine not found!");
        jobject toast = env->CallStaticObjectMethod(
            toastClass, makeText, activity->clazz, message, 1); // 1 = LENGTH_LONG
        
        jmethodID show = env->GetMethodID(toastClass, "show", "()V");
        env->CallVoidMethod(toast, show);
    }
    
    // 清理
    env->DeleteLocalRef(packageName);
    if (intent != nullptr) {
        env->DeleteLocalRef(intent);
    }
    env->DeleteLocalRef(packageManager);
    
    // 分离线程
    vm->DetachCurrentThread();
}

// 简单的渲染和输入处理
void handle_input(android_app* app) {
    AInputEvent* event = nullptr;
    
    while (AInputQueue_getEvent(app->inputQueue, &event) >= 0) {
        if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
            continue;
        }
        
        int32_t type = AInputEvent_getType(event);
        int32_t action = AMotionEvent_getAction(event);
        
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            if (action == AMOTION_EVENT_ACTION_DOWN) {
                LOGI("Touch detected, launching NovaFlareEngine...");
                
                // 在新线程中启动应用程序，避免阻塞UI
                std::thread launchThread(launchNovaFlareEngine, app->activity);
                launchThread.detach();
            }
        }
        
        AInputQueue_finishEvent(app->inputQueue, event);
    }
}

void android_main(android_app* app) {
    app->onInputEvent = [](android_app* app, AInputEvent* event) -> int32_t {
        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event);
            if (action == AMOTION_EVENT_ACTION_DOWN) {
                LOGI("Button pressed, launching NovaFlareEngine...");
                
                // 在新线程中启动
                std::thread(launchNovaFlareEngine, app->activity).detach();
                return 1;
            }
        }
        return 0;
    };

    // 主循环
    while (true) {
        int events;
        android_poll_source* source;
        
        // 处理所有事件
        while (ALooper_pollAll(0, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            
            // 检查应用是否正在退出
            if (app->destroyRequested != 0) {
                LOGI("App is exiting...");
                return;
            }
        }
        
        // 简单的渲染 - 清除为蓝色背景
        if (app->window != nullptr) {
            ANativeWindow_Buffer buffer;
            if (ANativeWindow_lock(app->window, &buffer, nullptr) == 0) {
                // 填充为蓝色
                uint32_t* pixels = static_cast<uint32_t*>(buffer.bits);
                for (int i = 0; i < buffer.width * buffer.height; ++i) {
                    pixels[i] = 0xFF0000FF; // ARGB: 蓝色
                }
                ANativeWindow_unlockAndPost(app->window);
            }
        }
    }
}
