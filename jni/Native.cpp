/*
 * Native.cpp
 *
 *  Created on: 2016年1月16日
 *      Author: peng
 */
#include <string.h>
#include <errno.h>
#include <limits.h>	// PATH_MAX
#include <fcntl.h>  // open()
#include "dexdump/DexDump.cpp"
#include "libdvm/DvmDex.h"
#include "libdex/DexFile.h"
#include "AndroidHelper.h"

namespace JNI{

	#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
	static const char *gClassPathName = "com/tomagoyaky/TestUtil";
	static const char *gNativeBridgeClassPathName = "com/ClassLoaderShell/ShellEntry";
	static const char *gDexFilePath = "/sdcard/classes.dex";
	static int dexfd = 0;
	DvmDex *pDvmDex;

	static void jni_DexClassLoader(JNIEnv* env, jobject thiz, jobject context){
		jstring jDexFilePath = env->NewStringUTF(gDexFilePath);

		if(access(gDexFilePath, R_OK) == -1){
			LOGE("dex file {%s} is not exist.", gDexFilePath);
			return;
		}

		dexfd = open(gDexFilePath, O_RDONLY);
		LOGD("dexfd:%d", dexfd);
		if(dexfd != -1){
			int result = dvmDexFileOpenFromFd(dexfd, &pDvmDex);
			LOGD("load dex file %s, pDvmDex->pDexFile->baseAddr:%p, result:%d", gDexFilePath, pDvmDex->pDexFile->baseAddr, result);
		}
		LOGD("jni_DexClassLoader");
	}

	static void jni_ResumeApplication(JNIEnv* env, jobject thiz, jobject context){
		LOGD("jni_ResumeApplication");
	}

	static void jni_DexProcess(JNIEnv* env, jobject thiz, jstring dexFilePath){
		LOGD("jni_DexProcess");

		const char  *classDescriptor;
		DexFile *pDexFile = pDvmDex->pDexFile;
		dumpFileHeader(pDexFile);

		LOGI("===========================================================");
		for (int idx = 0; idx < pDexFile->pHeader->classDefsSize; ++idx) {
			//dumpClass(pDvmDex->pDexFile, i, NULL);

			const DexClassDef* pClassDef = dexGetClassDef(pDexFile, idx);
			if (pClassDef == NULL) {
				LOGW("Trouble reading classDef\n");
				return;
			}
			const char *accessStr = createAccessFlagStr(pClassDef->accessFlags, kAccessForClass);
			const char *classDescriptor = dexStringByTypeIdx(pDexFile, pClassDef->classIdx);
			const char *superclassDescriptor = dexStringByTypeIdx(pDexFile, pClassDef->superclassIdx);
			// 过滤掉系统类
			if(strncmp(classDescriptor, "Landroid/support/", strlen("Landroid/support/")) != 0){
				LOGW("  Class #%d : %s '%s' <- %s (superclass)\n", idx, accessStr, classDescriptor, superclassDescriptor);
				const u1* pEncodedData = dexGetClassData(pDexFile, pClassDef);
				if (pEncodedData == NULL) {
					LOGE("Trouble reading pEncodedData\n");
					return;
				}

				DexClassData* pClassData = dexReadAndVerifyClassData(&pEncodedData, NULL);

				if (pClassData == NULL) {
					LOGE("Trouble reading pClassData\n");
					return;
				}

				for (int i = 0; i < (int) pClassData->header.directMethodsSize; i++) {
					DexMethod dexMethod = pClassData->directMethods[i];
					const char *name = dexStringById(pDexFile, dexMethod.methodIdx);
					LOGD(" Direct methods name          : '%s'\n", name);
				}

				for (int i = 0; i < (int) pClassData->header.virtualMethodsSize; i++) {
					DexMethod dexMethod = pClassData->directMethods[i];
					const char *name = dexStringById(pDexFile, dexMethod.methodIdx);
					LOGD(" Virtual methods name          : '%s'\n", name);
				}
			}
		}
	}

	// XXX 0x3 注册本地方法
	const JNINativeMethod method_table[] = {
			{"DexProcess", "(Ljava/lang/String;)V", (void*)jni_DexProcess},
			{"DexClassLoaderWithNative", "(Landroid/content/Context;)V", (void*)jni_DexClassLoader},
			{"ResumeApplicationWithNative", "(Landroid/content/Context;)V", (void*)jni_ResumeApplication}
	};

	int registerNativeMethod(JNIEnv *env){

		// 获取类
		jclass clazz = env->FindClass(gNativeBridgeClassPathName);

		if (clazz == NULL){
			LOGE("ERROR: unable to find class '%s'", (char*)gNativeBridgeClassPathName);
			return JNI_ERR;
		}
		return env->RegisterNatives(clazz, method_table, NELEM(method_table));
	}
}

