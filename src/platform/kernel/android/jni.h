/**
 * @file jni.h
 * @brief Position-independent JNI bridge for Android ART VM
 *
 * @details Minimal JNI type definitions and a bridge to attach to the
 * running ART Java VM from position-independent native code. The PIC
 * agent runs as a thread in the APK's process (loaded via libc2.so),
 * so ART is already initialized — we just need to find it and attach.
 *
 * Resolution chain:
 *   /proc/self/maps → libart.so base → ELF .dynsym →
 *   JNI_GetCreatedJavaVMs → JavaVM → AttachCurrentThread → JNIEnv
 *
 * Once we have JNIEnv, we can call any Java method via JNI reflection:
 *   FindClass, GetMethodID, CallObjectMethod, etc.
 *
 * This is the Android equivalent of macOS dyld.cc (which resolves
 * CoreGraphics functions at runtime for screen capture).
 *
 * @ingroup kernel_android
 */

#pragma once

#include "core/types/primitives.h"

// =============================================================================
// JNI type definitions (from jni.h, minimal subset)
// =============================================================================

/// @brief Opaque Java object reference
using jobject    = PVOID;
using jclass     = jobject;
using jstring    = jobject;
using jmethodID  = PVOID;
using jfieldID   = PVOID;
using jint       = INT32;
using jlong      = INT64;
using jboolean   = UINT8;
using jsize      = INT32;
using jbyteArray = jobject;
using jintArray  = jobject;
using jobjectArray = jobject;

/// @brief JNI value union for method calls
union jvalue
{
	jboolean z;
	INT8     b;
	UINT16   c;
	INT16    s;
	jint     i;
	jlong    j;
	float    f;
	double   d;
	jobject  l;
};

// =============================================================================
// JNI function table (JNIEnv) — minimal subset for our needs
// =============================================================================

/// @brief JNI Native Interface function table
/// @details This mirrors the JNINativeInterface_ struct from jni.h.
/// Only the slots we need are typed; the rest are void* placeholders.
/// The JNI spec guarantees stable function table offsets across versions.
struct JNINativeInterface
{
	PVOID reserved0;
	PVOID reserved1;
	PVOID reserved2;
	PVOID reserved3;

	jint    (*GetVersion)(PVOID env);                                          // 4
	jclass  (*DefineClass)(PVOID, const CHAR *, jobject, const INT8 *, jsize); // 5
	jclass  (*FindClass)(PVOID env, const CHAR *name);                         // 6
	PVOID   fn7;  // FromReflectedMethod
	PVOID   fn8;  // FromReflectedField
	PVOID   fn9;  // ToReflectedMethod
	jclass  (*GetSuperclass)(PVOID env, jclass clazz);                         // 10
	PVOID   fn11; // IsAssignableFrom
	PVOID   fn12; // ToReflectedField
	PVOID   fn13; // Throw
	PVOID   fn14; // ThrowNew
	jobject (*ExceptionOccurred)(PVOID env);                                   // 15
	PVOID   fn16; // ExceptionDescribe
	void    (*ExceptionClear)(PVOID env);                                      // 17
	PVOID   fn18; // FatalError
	PVOID   fn19; // PushLocalFrame
	PVOID   fn20; // PopLocalFrame
	jobject (*NewGlobalRef)(PVOID env, jobject obj);                           // 21
	void    (*DeleteGlobalRef)(PVOID env, jobject globalRef);                  // 22
	void    (*DeleteLocalRef)(PVOID env, jobject localRef);                    // 23
	PVOID   fn24; // IsSameObject
	PVOID   fn25; // NewLocalRef
	PVOID   fn26; // EnsureLocalCapacity
	PVOID   fn27; // AllocObject
	jobject (*NewObject)(PVOID env, jclass clazz, jmethodID methodID, ...);   // 28
	PVOID   fn29; // NewObjectV
	jobject (*NewObjectA)(PVOID env, jclass clazz, jmethodID methodID, const jvalue *args); // 30
	jclass  (*GetObjectClass)(PVOID env, jobject obj);                         // 31
	PVOID   fn32; // IsInstanceOf
	jmethodID (*GetMethodID)(PVOID env, jclass clazz, const CHAR *name, const CHAR *sig); // 33
	jobject (*CallObjectMethod)(PVOID env, jobject obj, jmethodID methodID, ...);         // 34
	PVOID   fn35; // CallObjectMethodV
	jobject (*CallObjectMethodA)(PVOID env, jobject obj, jmethodID methodID, const jvalue *args); // 36
	jboolean (*CallBooleanMethod)(PVOID env, jobject obj, jmethodID methodID, ...);       // 37
	PVOID   fn38; // CallBooleanMethodV
	PVOID   fn39; // CallBooleanMethodA
	PVOID   fn40; // CallByteMethod
	PVOID   fn41; // CallByteMethodV
	PVOID   fn42; // CallByteMethodA
	PVOID   fn43; // CallCharMethod
	PVOID   fn44; // CallCharMethodV
	PVOID   fn45; // CallCharMethodA
	PVOID   fn46; // CallShortMethod
	PVOID   fn47; // CallShortMethodV
	PVOID   fn48; // CallShortMethodA
	jint    (*CallIntMethod)(PVOID env, jobject obj, jmethodID methodID, ...);             // 49
	PVOID   fn50; // CallIntMethodV
	jint    (*CallIntMethodA)(PVOID env, jobject obj, jmethodID methodID, const jvalue *args); // 51
	PVOID   fn52; // CallLongMethod
	PVOID   fn53; // CallLongMethodV
	PVOID   fn54; // CallLongMethodA
	PVOID   fn55_70[16]; // Float/Double/Void method variants
	PVOID   fn71_111[41]; // NonvirtualCall*, GetField*, SetField*
	jmethodID (*GetStaticMethodID)(PVOID env, jclass clazz, const CHAR *name, const CHAR *sig); // 113
	jobject (*CallStaticObjectMethod)(PVOID env, jclass clazz, jmethodID methodID, ...);         // 114
	PVOID   fn115; // CallStaticObjectMethodV
	jobject (*CallStaticObjectMethodA)(PVOID env, jclass clazz, jmethodID methodID, const jvalue *args); // 116
	PVOID   fn117_124[8]; // CallStaticBool/Byte/Char/Short
	jint    (*CallStaticIntMethod)(PVOID env, jclass clazz, jmethodID methodID, ...);      // 125
	PVOID   fn126; // CallStaticIntMethodV
	PVOID   fn127; // CallStaticIntMethodA
	PVOID   fn128_135[8]; // CallStaticLong/Float/Double
	void    (*CallStaticVoidMethod)(PVOID env, jclass clazz, jmethodID methodID, ...);     // 136
	PVOID   fn137; // CallStaticVoidMethodV
	void    (*CallStaticVoidMethodA)(PVOID env, jclass clazz, jmethodID methodID, const jvalue *args); // 138
	jfieldID (*GetStaticFieldID)(PVOID env, jclass clazz, const CHAR *name, const CHAR *sig); // 139
	jobject (*GetStaticObjectField)(PVOID env, jclass clazz, jfieldID fieldID);             // 140
	PVOID   fn141_144[4]; // GetStaticBool/Byte/Char/Short
	jint    (*GetStaticIntField)(PVOID env, jclass clazz, jfieldID fieldID);                // 145
	PVOID   fn146_170[25]; // SetStatic*, NewString, GetStringLength, etc.
	jstring (*NewStringUTF)(PVOID env, const CHAR *bytes);                                  // 167 (actual index)
	PVOID   fn_rest[128]; // Remaining JNI functions (array ops, monitors, etc.)
};

/// @brief JNIEnv pointer — double-indirection to the function table
/// @details JNIEnv* points to a JNINativeInterface** (pointer to vtable pointer).
/// Usage: (*env)->FindClass(env, "java/lang/String")
using JNIEnv = JNINativeInterface *;

// =============================================================================
// JavaVM function table — minimal subset
// =============================================================================

/// @brief JavaVM invocation interface
struct JNIInvokeInterface
{
	PVOID reserved0;
	PVOID reserved1;
	PVOID reserved2;
	jint (*DestroyJavaVM)(PVOID vm);                                            // 3
	jint (*AttachCurrentThread)(PVOID vm, PVOID *penv, PVOID args);             // 4
	jint (*DetachCurrentThread)(PVOID vm);                                      // 5
	jint (*GetEnv)(PVOID vm, PVOID *penv, jint version);                        // 6
	jint (*AttachCurrentThreadAsDaemon)(PVOID vm, PVOID *penv, PVOID args);     // 7
};

/// @brief JavaVM pointer — double-indirection to invocation interface
using JavaVM = JNIInvokeInterface *;

/// @brief JNI version constant (1.6 — works on all Android versions)
constexpr jint JNI_VERSION_1_6 = 0x00010006;
constexpr jint JNI_OK = 0;
constexpr jint JNI_ERR = -1;

// =============================================================================
// JNI_GetCreatedJavaVMs function signature
// =============================================================================

/// @brief Signature for JNI_GetCreatedJavaVMs (resolved from libart.so)
using FnGetCreatedJavaVMs = jint (*)(JavaVM **vmBuf, jsize bufLen, jsize *nVMs);

// =============================================================================
// JNI Bridge API
// =============================================================================

/**
 * @brief Attach to the running ART Java VM and get a JNIEnv
 *
 * @details Finds libart.so in /proc/self/maps, resolves
 * JNI_GetCreatedJavaVMs, retrieves the VM, and attaches
 * the current thread. The returned JNIEnv is valid for the
 * calling thread's lifetime (or until DetachCurrentThread).
 *
 * @param outEnv [out] Receives the JNIEnv pointer on success
 * @param outVm  [out] Receives the JavaVM pointer on success (optional, may be nullptr)
 * @return true on success, false on failure
 */
BOOL JniBridgeAttach(JNIEnv **outEnv, JavaVM **outVm);
