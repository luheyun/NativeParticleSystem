#ifndef UNITYCONFIGURE_H
#define UNITYCONFIGURE_H

#include "config.h"

// ADD_NEW_PLATFORM_HERE: review this file

/// Increase this number by one if you want all users
/// running a project with a different version to rebuild the library
#define UNITY_REBUILD_LIBRARY_VERSION 11

// Increase this number if you want to prevent users from downgrading to an older version.
#define UNITY_FORWARD_COMPATIBLE_VERSION 40

// Increase this number of you want to tell people that now your project folder won't open
// in old Unity versions anymore
#define UNITY_ASK_FOR_UPGRADE_VERSION 40

/// Increase this number if you want the user the user to upgrade to the latest standard assets
#define UNITY_STANDARD_ASSETS_VERSION 0

#ifndef UNITY_EXTERNAL_TOOL
#define UNITY_EXTERNAL_TOOL 0
#endif

#ifndef ENABLE_DOTNET
#define ENABLE_DOTNET 0
#endif

#ifndef ENABLE_MONO
#define ENABLE_MONO 0
#endif

#ifndef ENABLE_IL2CPP
#define ENABLE_IL2CPP 0
#endif

#ifndef ENABLE_SERIALIZATION_BY_CODEGENERATION
#define ENABLE_SERIALIZATION_BY_CODEGENERATION 0
#endif

#if UNITY_EXTERNAL_TOOL
// Threaded loading is always off in external tools (e.g. BinaryToText)
#define THREADED_LOADING 0
#define ENABLE_THREAD_CHECK_IN_ALLOCS 0
#else
#define THREADED_LOADING !UNITY_WEBGL
#endif

#define THREADED_LOADING_DEBUG 0

#if !defined(SUPPORT_ENVIRONMENT_VARIABLES)
#define SUPPORT_ENVIRONMENT_VARIABLES (!UNITY_XENON && !UNITY_WINRT)
#endif // !defined(SUPPORT_ENVIRONMENT_VARIABLES)

#if !defined(SUPPORT_MULTIPLE_DISPLAYS)
#define SUPPORT_MULTIPLE_DISPLAYS ((UNITY_STANDALONE && ENABLE_MULTIPLE_DISPLAYS && !UNITY_METRO) || UNITY_ANDROID)
#endif // !defined(SUPPORT_MULTIPLE_DISPLAYS)

// Defines if platform supports threads and synchronization primitives.
// Currently only WebGL doesn't support it.
#ifndef SUPPORT_THREADS
#define SUPPORT_THREADS 1
#endif

// Allows subsystems to parallelize execution of their code. (Only make sense if platform supports threads)
#ifndef ENABLE_MULTITHREADED_CODE
#define ENABLE_MULTITHREADED_CODE SUPPORT_THREADS
#endif

#define MONO_2_10 0
#define MONO_2_12 (INCLUDE_MONO_2_12 && (UNITY_WIN || UNITY_OSX))
#define USE_MONO_DOMAINS (ENABLE_MONO && (UNITY_EDITOR || WEBPLUG))
#define ENABLE_SCRIPTING (!UNITY_EXTERNAL_TOOL && (ENABLE_DOTNET || ENABLE_IL2CPP || ENABLE_MONO))
#define ENABLE_UNITYGUI 1
#define ENABLE_JPG (!UNITY_WINWEB_COMMON)
#define ENABLE_PNG 1
#define ENABLE_WEBPLAYER_SECURITY ((UNITY_EDITOR || WEBPLUG) && ENABLE_MONO)
//#define ENABLE_MEMORY_MANAGER (!UNITY_EXTERNAL_TOOL || UNET_SERVER_LIB) // todo
#define ENABLE_WEBVIEW (UNITY_EDITOR && !UNITY_LINUX)
#define ENABLE_ASSET_STORE ENABLE_WEBVIEW
#define ENABLE_ASSET_SERVER (UNITY_EDITOR && !UNITY_LINUX)
#define ENABLE_LIGHTMAPPER (UNITY_EDITOR && !UNITY_LINUX)
#define FRAME_DEBUGGER_AVAILABLE (ENABLE_FRAME_DEBUGGER && (UNITY_EDITOR || UNITY_DEVELOPER_BUILD) && ENABLE_MULTITHREADED_CODE)

#ifndef UNITY_WINRT_API
#define UNITY_WINRT_API (UNITY_METRO_API || UNITY_EDITOR)
#endif

#define UNITY_WINRT_8_1 (UNITY_WP_8_1 || UNITY_METRO_8_1)

#define UNITY_POSIX (UNITY_OSX || UNITY_IPHONE || UNITY_TVOS || UNITY_ANDROID || UNITY_LINUX || UNITY_TIZEN || UNITY_STV)

// More information can be found here - http://msdn.microsoft.com/en-us/library/windows/apps/bg182880.aspx#five
#define ENABLE_DX11_FRAME_LATENCY_WAITABLE_OBJECT UNITY_WINRT

#if !defined(UNITY_DYNAMIC_TLS)
#if UNITY_XENON || UNITY_WINRT
#define UNITY_DYNAMIC_TLS 0
#else
#define UNITY_DYNAMIC_TLS 1
#endif
#endif // !defined(UNITY_DYNAMIC_TLS)

#ifndef SUPPORT_MONO_THREADS
#define SUPPORT_MONO_THREADS (ENABLE_MONO && SUPPORT_THREADS)
#endif

#ifndef SUPPORT_IL2CPP_THREADS
#define SUPPORT_IL2CPP_THREADS (ENABLE_IL2CPP && SUPPORT_THREADS)
#endif

#define SUPPORT_SCRIPTING_THREADS (SUPPORT_MONO_THREADS || SUPPORT_IL2CPP_THREADS)

#ifndef UNITY_EMULATE_PERSISTENT_DATAPATH
#define UNITY_EMULATE_PERSISTENT_DATAPATH ((UNITY_WIN && !UNITY_WINRT) || UNITY_OSX || UNITY_LINUX || UNITY_XENON)
#endif

// TODO: pass UNITY_DEVELOPER_BUILD instead of ENABLE_PROFILER from projects
// Defines whether this is a development player build (user checks "Development Player" in the build settings)
// It determines whether debugging symbols are built/used, profiler can be enabled, etc.
#if !defined(UNITY_DEVELOPER_BUILD)
#if defined(ENABLE_PROFILER)
	#define UNITY_DEVELOPER_BUILD ENABLE_PROFILER
#else
	#define UNITY_DEVELOPER_BUILD 1
#endif
#endif

#define UNITY_ISBLEEDINGEDGE_BUILD 0

#if !defined(PLATFORM_SUPPORTS_PROFILER)	// can be defined per platform
#define PLATFORM_SUPPORTS_PROFILER (UNITY_XENON || UNITY_OSX || UNITY_WIN || UNITY_LINUX || UNITY_ANDROID || UNITY_WINRT || UNITY_STV || UNITY_WEBGL || UNITY_XBOXONE || UNITY_TIZEN)
#endif // !defined(PLATFORM_SUPPORTS_PROFILER)


#define ENABLE_PROFILER_INTERNAL_CALLS 0

#define ENABLE_MONO_MEMORY_PROFILER (ENABLE_PROFILER && ENABLE_MONO)
#define ENABLE_IL2CPP_MEMORY_PROFILER (ENABLE_PROFILER && ENABLE_IL2CPP)
#define ENABLE_SCRIPTING_MEMORY_PROFILER (ENABLE_MONO_MEMORY_PROFILER || ENABLE_IL2CPP_MEMORY_PROFILER)
#define ENABLE_SCRIPTING_MEMORY_SNAPSHOTS (ENABLE_PROFILER && ENABLE_IL2CPP)
#define ENABLE_SCRIPTING_DEEP_PROFILER (ENABLE_PROFILER && UNITY_EDITOR)
#define ENABLE_EVENT_TRACING_FOR_WINDOWS (ENABLE_PROFILER && (UNITY_WINRT || (UNITY_WIN && UNITY_EDITOR))) // Works on Vista and up. Since we still support XP on standalone, it would require quite a bit of changes to load it dynamically.
#define ENABLE_ETW_SCRIPTING (ENABLE_EVENT_TRACING_FOR_WINDOWS && UNITY_METRO && !UNITY_WP_8_1)	// Only enable ETW scripting events on Windows in official builds because those are useless for users on the phone,
																			// as there are no consumer level tools to extract them (we can do it with TShell, though)
#define ENABLE_MONO_MEMORY_CALLBACKS 0
#define ENABLE_IL2CPP_MEMORY_CALLBACKS 0

// Cannot enable on Android, as it overrides callbacks in Android_Profiler.cpp
//Be VERY careful when turning on heapshot profiling. we disabled it on all platforms for now, because it was causing flipflop deadlocks in windows gfx tests only in multithreaded mode.
//usually around test 72. if you enable this, make sure to do many runs of wingfxtests and ensure you didn't introdue a flipflopping deadlock.
#define ENABLE_MONO_HEAPSHOT 0
#define ENABLE_MONO_HEAPSHOT_GUI 0

#define UNITY_NO_UNALIGNED_MEMORY_ACCESS (UNITY_WEBGL || (UNITY_ANDROID && defined(__arm__)))

#if !defined(PLATFORM_SUPPORTS_PLAYERCONNECTION)
#define PLATFORM_SUPPORTS_PLAYERCONNECTION 1
#define PLATFORM_SUPPORTS_PLAYERCONNECTION_LISTENING (!UNITY_WEBGL)
#endif // !defined(PLATFORM_SUPPORTS_PLAYERCONNECTION)
#ifdef ENABLE_PLAYERCONNECTION
#undef ENABLE_PLAYERCONNECTION
#endif
#define ENABLE_PLAYERCONNECTION (UNITY_DEVELOPER_BUILD && PLATFORM_SUPPORTS_PLAYERCONNECTION)

#define ENABLE_SOCKETS (ENABLE_NETWORK || ENABLE_PLAYERCONNECTION)

// MONO_QUALITY_ERRORS are only for the editor!
#define MONO_QUALITY_ERRORS UNITY_EDITOR

// ENABLE_SCRIPTING_API_THREAD_AND_SERIALIZATION_CHECK are only for the editor and development/debug builds!
#if !defined(ENABLE_SCRIPTING_API_THREAD_AND_SERIALIZATION_CHECK)
#define ENABLE_SCRIPTING_API_THREAD_AND_SERIALIZATION_CHECK (UNITY_EDITOR || ((UNITY_DEVELOPER_BUILD || DEBUGMODE) && SUPPORT_THREADS )) && (ENABLE_DOTNET || ENABLE_MONO|| ENABLE_IL2CPP)
#endif

// New Unload Unused Assets implementation doesn't work on WinRT .NET, that's why we use old implementation
#define ENABLE_OLD_GARBAGE_COLLECT_SHARED_ASSETS ENABLE_DOTNET

#if !defined(SUPPORT_MOUSE)
#define SUPPORT_MOUSE (!UNITY_XENON)
#endif //!defined(SUPPORT_MOUSE)



#define SUPPORT_ERROR_EXIT WEBPLUG
#define SUPPORT_DIRECT_FILE_ACCESS 1
#define HAS_DXT_DECOMPRESSOR (!UNITY_IPHONE && !UNITY_TVOS)

#define HAS_PVRTC_DECOMPRESSOR (UNITY_EDITOR || UNITY_ANDROID || UNITY_IPHONE || UNITY_TVOS || UNITY_STV)
#define HAS_ETC_DECOMPRESSOR (UNITY_EDITOR || UNITY_ANDROID || UNITY_IPHONE || UNITY_TVOS || UNITY_TIZEN || UNITY_STV)
#define HAS_ATC_DECOMPRESSOR (UNITY_EDITOR || UNITY_ANDROID)
#define HAS_ASTC_DECOMPRESSOR (UNITY_EDITOR || UNITY_ANDROID || UNITY_IPHONE || UNITY_TVOS)

#define UNITY_APPLE_PVR (UNITY_IPHONE || UNITY_TVOS)

#ifndef UNITY_USE_PREFIX_EXTERN_SYMBOLS
#define UNITY_USE_PREFIX_EXTERN_SYMBOLS (UNITY_ANDROID || UNITY_WINRT || UNITY_TIZEN || UNITY_STV || UNITY_PSP2)
#endif

/// Should we output errors when writing serialized files with wrong alignment on a low level.
/// (ProxyTransfer should find all alignment issues in theory)
#define CHECK_SERIALIZE_ALIGNMENT 0 /////@TODO: WHY IS THIS NOT ENABLED?????

#define SUPPORT_SERIALIZE_WRITE UNITY_EDITOR
#define SUPPORT_TEXT_SERIALIZATION UNITY_EDITOR

#define SUPPORT_SERIALIZED_TYPETREES !ENABLE_SERIALIZATION_BY_CODEGENERATION

#ifndef UNITY_IPHONE_AUTHORING
#define UNITY_IPHONE_AUTHORING UNITY_EDITOR && UNITY_OSX
#endif

#ifndef UNITY_ANDROID_AUTHORING
#define UNITY_ANDROID_AUTHORING UNITY_EDITOR
#endif

#if !defined(USE_MONO_AOT)	// can be defined per platform
#if ENABLE_IL2CPP
# define USE_MONO_AOT 0
#else
#define USE_MONO_AOT (UNITY_XENON || UNITY_XBOXONE )
#endif
#endif //!defined(USE_MONO_AOT)

// Flag defined in mono, when AOT libraries are built with -ficall option
// But that is not available in mono/consoles
#ifndef USE_MONO_AOT_FASTMETHOD
#define USE_MONO_AOT_FASTMETHOD (USE_MONO_AOT && !(UNITY_XENON || UNITY_PS3 || UNITY_PSP2 || UNITY_PS4 || UNITY_XBOXONE))
#endif

#define IL2CPP_NO_EXCEPTIONS (ENABLE_IL2CPP && ((UNITY_IPHONE && !TARGET_IPHONE_SIMULATOR) || (UNITY_TVOS && !TARGET_TVOS_SIMULATOR)))

// CODE STRIPPING
// Build post-processors generate i-call registrations
#define INTERNAL_CALL_STRIPPING (UNITY_IPHONE || UNITY_TVOS || UNITY_WEBGL)
// Build post-processors generate class registrations
#define SUPPORTS_GRANULAR_CLASS_REGISTRATION (UNITY_WEBGL || ((UNITY_IPHONE || UNITY_TVOS) && ENABLE_IL2CPP))
// Build post-processors generate module registrations
#define SUPPORTS_GRANULAR_MODULE_REGISTRATION (SUPPORTS_GRANULAR_CLASS_REGISTRATION)
// Managed types are stripped when not used in user scripts
#define SUPPORTS_MANAGED_CODE_STRIPPING ((UNITY_IPHONE || UNITY_TVOS || UNITY_XENON) && !ENABLE_IL2CPP) || UNITY_WEBGL

// CODE MODULARIZATION
// Modules are allowed to register i-calls
#define SUPPORTS_MODULE_INTERNAL_CALL_REGISTRATION (ENABLE_MONO || ENABLE_DOTNET || ENABLE_IL2CPP)

// IL2CPP
#ifndef LOAD_IL2CPP_DYNAMICALLY
#define LOAD_IL2CPP_DYNAMICALLY !UNITY_WEBGL && !UNITY_IPHONE && !UNITY_TVOS
#endif

// Leaving AOT targets disabled for now - debugging on AOT will _at least_ require additional AOT compilation arguments
// On platforms where we don't have unity profiler capabilities, this is still useful because it loads mono debugging info which let's us resolve IL offsets during crashes
// On Xbox we use this to load debugging info for resolving stack traces
#ifndef USE_MONO_DEBUGGER
#define USE_MONO_DEBUGGER (UNITY_DEVELOPER_BUILD && (!USE_MONO_AOT || UNITY_IPHONE || UNITY_TVOS)) || ((UNITY_PSP2 || UNITY_PS4 || UNITY_XBOXONE || UNITY_XENON) && !MASTER_BUILD)
#endif

#ifndef USE_CONSOLEBRANCH_MONO
#define USE_CONSOLEBRANCH_MONO (UNITY_XBOXONE || UNITY_XENON || UNITY_PS3 || UNITY_PSP2 || UNITY_PS4 )
#endif

#ifndef PLAY_QUICKTIME_DIRECTLY
#define PLAY_QUICKTIME_DIRECTLY 0
#endif

#define SCRIPTINGAPI_CAN_STORE_UNITYCLASSINFO_ON_SYSTEMTYPEINSTANCE ((ENABLE_MONO && !USE_CONSOLEBRANCH_MONO) || ENABLE_DOTNET)

// Do we have iPhone remote support (which is only on the host PC and only in the editor)
#define SUPPORT_IPHONE_REMOTE (UNITY_IPHONE_AUTHORING || UNITY_ANDROID_AUTHORING)

#define ENABLE_SECURITY WEBPLUG || UNITY_EDITOR || UNITY_STV
#define ENABLE_CORECLR_SECURITY (WEBPLUG || UNITY_STV)
#define ENABLE_CORECLR_PLUGIN_SECURITY (WEBPLUG)

#define ENABLE_CORECLR_SOCKET_SECURITY (WEBPLUG)

#define ALLOW_CLASS_ID_STRIPPING !UNITY_EDITOR

#ifndef ENABLE_MEMORY_TRACKING
// ENABLE_MEMORY_TRACKING is not enabled by default, because it makes debug build very slow
// we enable it for certain builds from a command line
#define ENABLE_MEMORY_TRACKING 0
#endif

// Whether to enable the unit test suite.
#ifndef ENABLE_UNIT_TESTS
#define ENABLE_UNIT_TESTS (UNITY_EDITOR || (UNITY_DEVELOPER_BUILD && (UNITY_STANDALONE || UNITY_ANDROID || UNITY_XENON || UNITY_PS4 || UNITY_WINRT || UNITY_XBOXONE)))
#endif

// Whether to enable faking hooks. Faking currently requires decltype() and RTTI support.
// Should never be shipped as it doesn't just add test code to the binary but actually modifies the code being tested.
// ATM the script faking only supports Mono so we disable the tests entirely otherwise for now.
// Also, we currently disable RTTI in players so for now fakes are restricted to the editor.
#ifndef ENABLE_UNIT_TESTS_WITH_FAKES
#if UNITY_EXTERNAL_TOOL
#define ENABLE_UNIT_TESTS_WITH_FAKES 0
#else
#define ENABLE_UNIT_TESTS_WITH_FAKES (SUPPORT_CPP0X_DECLTYPE && SUPPORT_CPP_RTTI && ENABLE_UNIT_TESTS && ENABLE_MONO)
#endif
#endif

// Whether to enable the performance test suite. We have it enabled in all builds that have native tests enabled in
// general to make sure that devs don't break the compile of these tests in their ABV but we actually run the tests
// only against shipping builds.
#ifndef ENABLE_PERFORMANCE_TESTS
#define ENABLE_PERFORMANCE_TESTS (UNITY_EDITOR || (UNITY_DEVELOPER_BUILD && (UNITY_STANDALONE || UNITY_ANDROID || UNITY_XENON || UNITY_PS4 || UNITY_XBOXONE)))
#endif

// If either unit or performance tests are enabled, make sure we enable the native test framework.
// NOTE: At the moment, we ship at least some of the tests and testing infrastructure. This is because we
//       want to run some tests against the very configuration we ship and removing the tests from the final binary
//       would require yet another set of builds on the farm. Right now, we only ship performance tests which may
//       even prove useful as a form of benchmarking on user machines.
#define ENABLE_NATIVE_TEST_FRAMEWORK (ENABLE_UNIT_TESTS || ENABLE_PERFORMANCE_TESTS)

#define SUPPORT_RESOURCE_IMAGE_LOADING (!UNITY_EDITOR && !WEBPLUG && !UNITY_WEBGL)
#define SUPPORT_SERIALIZATION_FROM_DISK 1

#ifndef UNITY_CAN_SHOW_SPLASH_SCREEN
#define UNITY_CAN_SHOW_SPLASH_SCREEN (!UNITY_EDITOR && !WEBPLUG && !UNITY_XBOXONE)
#endif

#define ENABLE_CUSTOM_ALLOCATORS_FOR_STDMAP (!UNITY_WEBGL && !UNITY_LINUX && !((UNITY_OSX || UNITY_IOS) && UNITY_64))

#ifndef UNITY_FBX_IMPORTER
#define UNITY_FBX_IMPORTER 0
#endif

#ifndef JAM_BUILD
#define JAM_BUILD 0
#endif

#if !defined(UNITY_PLUGINS_AVAILABLE)
	#define UNITY_PLUGINS_AVAILABLE (!WEBPLUG && !UNITY_WEBGL)
#endif

#if !defined(UNITY_PLUGINS_ARE_IN_EXTERNAL_EXECUTABLE)
	#define UNITY_PLUGINS_ARE_IN_EXTERNAL_EXECUTABLE UNITY_PLUGINS_AVAILABLE
#endif

#if UNITY_WINRT
#define SCRIPT_BINDINGS_EXPORT_DECL
#if ENABLE_WINRT_PINVOKE
#	define SCRIPT_CALL_CONVENTION
#else
#	define SCRIPT_CALL_CONVENTION WINAPI
#endif
#else
#define SCRIPT_BINDINGS_EXPORT_DECL
#define SCRIPT_CALL_CONVENTION
#endif


// On some platforms, we need to use our own quarantined FreeType2 library to avoid
// conflicting symbols when linking against other libraries with their own FT symbols.
#define USE_EXTERNAL_FREETYPE (UNITY_TIZEN || STV_STANDARD_13 || STV_STANDARD_14)

#if UNITY_PS3
#  define QUIT_ON_DISC_EJECTION 1
#else
#  define QUIT_ON_DISC_EJECTION 0
#endif

/*There are alignment problems that cause issues with MurmurHash and CRC32 on WebGL, which prevents using a hashmap for the cache. 
AnimationEvent strings (part of the key) and std::pair<ScriptingClassPtr, int> (the key for the dense_hash_map) cause unaligned void* issues when hashed
The string issue could possibly be fixed by having a char*-specific hash method, and the std::pair method might be fixed by using a struct as the key instead.
For now, the cache is disabled on WebGL
*/
#if !UNITY_WEBGL
#	define USE_LAZY_SCRIPT_CACHE 1
#else
#	define USE_LAZY_SCRIPT_CACHE 0
#endif

#if !defined(USING_NAMESPACE_STD_IS_DEPRECATED)
//#  define USING_NAMESPACE_STD_IS_DEPRECATED		using namespace std	// allowed for now but deprecated, will be removed soon (transitional macro)
#  define USING_NAMESPACE_STD_IS_DEPRECATED	// expands to nothing, requires full qualification of std:: symbols or "using namespace std;" in an inner scope (not at file scope)
#endif

#endif
