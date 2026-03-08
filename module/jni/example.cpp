#include <android/log.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <jni.h>
#include "zygisk.hpp"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "BPM_Injector", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BPM_Injector", __VA_ARGS__)

#define TARGET_PACKAGE "com.skullcapstudios.bpm"
#define LIBRARY_PATH "/data/local/tmp/libmain.so"

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        const char *process = nullptr;
        if (args->nice_name != nullptr) {
            process = env->GetStringUTFChars(args->nice_name, nullptr);
        } else if (args->app_data_dir != nullptr) {
            process = env->GetStringUTFChars(args->app_data_dir, nullptr);
        }

        if (process && strstr(process, TARGET_PACKAGE)) {
            LOGD("Найдена целевая игра: %s", process);

            if (access(LIBRARY_PATH, F_OK) == -1) {
                LOGE("Библиотека не найдена по пути: %s", LIBRARY_PATH);
                goto cleanup;
            }

            void *handle = dlopen(LIBRARY_PATH, RTLD_NOW);
            if (handle) {
                LOGD("Библиотека успешно загружена!");
                void (*init_func)() = (void (*)()) dlsym(handle, "init");
                if (init_func) {
                    LOGD("Вызываем функцию init()");
                    init_func();
                }
            } else {
                LOGE("Ошибка загрузки библиотеки: %s", dlerror());
            }
        }

    cleanup:
        if (args->nice_name != nullptr && process != nullptr) {
            env->ReleaseStringUTFChars(args->nice_name, process);
        } else if (args->app_data_dir != nullptr && process != nullptr) {
            env->ReleaseStringUTFChars(args->app_data_dir, process);
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {}

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;
};

REGISTER_ZYGISK_MODULE(MyModule)
