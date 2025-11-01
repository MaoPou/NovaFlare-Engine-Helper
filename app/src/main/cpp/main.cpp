#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>
#include <string>
#include <thread>
#include <atomic>

#define LOG_TAG "NovaFlareHelper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::atomic<bool> g_launchRequested(false);

// 启动 NovaFlareEngine 的函数
void launchNovaFlareEngine(ANativeActivity* activity) {
    LOGI("Attempting to launch NovaFlareEngine...");
    
    JNIEnv* env = activity->env;
    JavaVM* vm = activity->vm;
    
    // 附加当前线程到 JVM
    jint result = vm->AttachCurrentThread(&env, nullptr);
    if (result != JNI_OK) {
        LOGE("Failed to attach thread to JVM");
        return;
    }
    
    jclass contextClass = env->GetObjectClass(activity->clazz);
    if (contextClass == nullptr) {
        LOGE("Failed to get context class");
        vm->DetachCurrentThread();
        return;
    }
    
    // 获取 getPackageManager 方法
    jmethodID getPackageManager = env->GetMethodID(
        contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    
    if (getPackageManager == nullptr) {
        LOGE("Failed to get getPackageManager method");
        env->DeleteLocalRef(contextClass);
        vm->DetachCurrentThread();
        return;
    }
    
    // 调用 getPackageManager
    jobject packageManager = env->CallObjectMethod(activity->clazz, getPackageManager);
    if (packageManager == nullptr) {
        LOGE("Failed to get PackageManager");
        env->DeleteLocalRef(contextClass);
        vm->DetachCurrentThread();
        return;
    }
    
    // 获取 PackageManager 类
    jclass pmClass = env->GetObjectClass(packageManager);
    if (pmClass == nullptr) {
        LOGE("Failed to get PackageManager class");
        env->DeleteLocalRef(packageManager);
        env->DeleteLocalRef(contextClass);
        vm->DetachCurrentThread();
        return;
    }
    
    // 获取 getLaunchIntentForPackage 方法
    jmethodID getLaunchIntent = env->GetMethodID(
        pmClass, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
    
    if (getLaunchIntent == nullptr) {
        LOGE("Failed to get getLaunchIntentForPackage method");
        env->DeleteLocalRef(pmClass);
        env->DeleteLocalRef(packageManager);
        env->DeleteLocalRef(contextClass);
        vm->DetachCurrentThread();
        return;
    }
    
    // 创建包名字符串
    jstring packageName = env->NewStringUTF("com.NovaFlareEngine");
    
    // 获取启动 Intent
    jobject intent = env->CallObjectMethod(packageManager, getLaunchIntent, packageName);
    
    if (intent != nullptr) {
        LOGI("Found NovaFlareEngine, starting activity...");
        
        // 获取 Context 类中的 startActivity 方法
        jmethodID startActivity = env->GetMethodID(
            contextClass, "startActivity", "(Landroid/content/Intent;)V");
        
        if (startActivity != nullptr) {
            // 启动活动
            env->CallVoidMethod(activity->clazz, startActivity, intent);
            LOGI("NovaFlareEngine launched successfully!");
        } else {
            LOGE("Failed to get startActivity method");
        }
        
        env->DeleteLocalRef(intent);
    } else {
        LOGE("NovaFlareEngine not found or not installed!");
        
        // 显示错误消息
        jclass toastClass = env->FindClass("android/widget/Toast");
        if (toastClass != nullptr) {
            jmethodID makeText = env->GetStaticMethodID(
                toastClass, "makeText", 
                "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;");
            
            if (makeText != nullptr) {
                jstring message = env->NewStringUTF("NovaFlareEngine not found!");
                jobject toast = env->CallStaticObjectMethod(
                    toastClass, makeText, activity->clazz, message, 1); // 1 = LENGTH_LONG
                
                if (toast != nullptr) {
                    jmethodID show = env->GetMethodID(toastClass, "show", "()V");
                    if (show != nullptr) {
                        env->CallVoidMethod(toast, show);
                    }
                    env->DeleteLocalRef(toast);
                }
                env->DeleteLocalRef(message);
            }
            env->DeleteLocalRef(toastClass);
        }
    }
    
    // 清理
    env->DeleteLocalRef(packageName);
    env->DeleteLocalRef(pmClass);
    env->DeleteLocalRef(packageManager);
    env->DeleteLocalRef(contextClass);
    
    // 分离线程
    vm->DetachCurrentThread();
    g_launchRequested = false;
}

void handle_cmd(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("Window initialized");
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("Window terminated");
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("Gained focus");
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("Lost focus");
            break;
    }
}

int32_t handle_input(android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        if (action == AMOTION_EVENT_ACTION_DOWN) {
            if (!g_launchRequested.exchange(true)) {
                LOGI("Touch detected, launching NovaFlareEngine...");
                
                // 在新线程中启动应用程序，避免阻塞UI
                std::thread launchThread(launchNovaFlareEngine, app->activity);
                launchThread.detach();
            }
            return 1;
        }
    }
    return 0;
}

void android_main(android_app* app) {
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

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
    }
}
