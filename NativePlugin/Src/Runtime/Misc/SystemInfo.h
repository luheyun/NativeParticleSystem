#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <string>

// ADD_NEW_PLATFORM_HERE
// NOTE: also match with enum in BaseClass.bindings
enum RuntimePlatform
{
	// NEVER change constants of existing platforms!
	kOSXEditor = 0,
	kOSXPlayer = 1,
	kWindowsPlayer = 2,
	kOSXWebPlayer = 3,
	kOSXDashboardPlayer = 4,
	kWindowsWebPlayer = 5,
	kWindowsEditor = 7,
	kiPhonePlayer = 8,
	kPS3Player = 9,
	kXenonPlayer = 10,
	kAndroidPlayer = 11,
	//kNaClWebPlayer = 12, //discontinued
	kLinuxPlayer = 13,
	kLinuxWebPlayer = 14,
	//kFlashPlayer = 15,   //discontinued
	kLinuxEditor = 16,
	kWebGLPlayer = 17,
	kMetroPlayerX86 = 18,
	kMetroPlayerX64 = 19,
	kMetroPlayerARM = 20,
	//kWP8Player = 21,  //discontinued
	kBlackBerryPlayer = 22,
	kTizenPlayer = 23,
	kPSP2Player = 24,
	kPS4Player = 25,
	kPSMPlayer = 26,
	kXboxOnePlayer=27,
	kSamsungTVPlayer = 28,
	kN3DSPlayer = 29,
	WiiUPlayer = 30,
	ktvOSPlayer = 31,

	kRuntimePlatformCount // keep this last
};

// ADD_NEW_OPERATING_SYSTEM_FAMILY_HERE
enum OperatingSystemFamily
{
	// NEVER change constants of existing operating system family!
	kUninitializedFamily = -1,
	kOtherFamily = 0,
	kMacOSXFamily = 1,
	kWindowsFamily = 2,
	kLinuxFamily = 3,

	kOperatingSystemFamilyCount // keep this last
};

enum DeviceType
{
	kDeviceTypeUnknown = 0,
	kDeviceTypeHandheld,
	kDeviceTypeConsole,
	kDeviceTypeDesktop,
	kDeviceTypeCount // keep this last
};

enum SystemLanguage {
	SystemLanguageAfrikaans,
	SystemLanguageArabic,
	SystemLanguageBasque,
	SystemLanguageBelarusian,
	SystemLanguageBulgarian,
	SystemLanguageCatalan,
	SystemLanguageChinese,
	SystemLanguageCzech,
	SystemLanguageDanish,
	SystemLanguageDutch,
	SystemLanguageEnglish,
	SystemLanguageEstonian,
	SystemLanguageFaroese,
	SystemLanguageFinnish,
	SystemLanguageFrench,
	SystemLanguageGerman,
	SystemLanguageGreek,
	SystemLanguageHebrew,
	SystemLanguageHungarian,
	SystemLanguageIcelandic,
	SystemLanguageIndonesian,
	SystemLanguageItalian,
	SystemLanguageJapanese,
	SystemLanguageKorean,
	SystemLanguageLatvian,
	SystemLanguageLithuanian,
	SystemLanguageNorwegian,
	SystemLanguagePolish,
	SystemLanguagePortuguese,
	SystemLanguageRomanian,
	SystemLanguageRussian,
	SystemLanguageSerboCroatian,
	SystemLanguageSlovak,
	SystemLanguageSlovenian,
	SystemLanguageSpanish,
	SystemLanguageSwedish,
	SystemLanguageThai,
	SystemLanguageTurkish,
	SystemLanguageUkrainian,
	SystemLanguageVietnamese,
	SystemLanguageChineseSimplified,
	SystemLanguageChineseTraditional,
	SystemLanguageUnknown
};

namespace systeminfo {

	extern const char* kUnsupportedIdentifier;
	
	std::string GetOperatingSystem();
	OperatingSystemFamily GetOperatingSystemFamily();
	std::string GetProcessorType();
	int GetProcessorFrequencyMHz();
	int EXPORT_COREMODULE GetProcessorCount();
	int GetPhysicalProcessorCount();
	int GetPhysicalMemoryMB();
	int GetUsedVirtualMemoryMB();
	int GetExecutableSizeMB();
	int GetSystemLanguage();
	std::string GetSystemLanguageCulture(int language);
	std::string GetSystemLanguageString(int language);
	std::string GetSystemLanguageISO();
	SystemLanguage ISOToSystemLanguage(std::string& langTag);

#if UNITY_XBOXONE
	ULONG GetCoreAffinityMask(int core);
#endif

#if UNITY_WIN
	ULONG_PTR GetCoreAffinityMask(DWORD core);
	std::string GetBIOSIdentifier();
#endif

#if UNITY_XENON
	void SetExecutableSizeMB(UInt32 sizeMB);
#endif

#if UNITY_WIN || UNITY_OSX || UNITY_LINUX
// Windows: 500=2000, 510=XP, 520=2003, 600=Vista, 610=7, 620=8, 630=8.1
// Mac: 1006=Snow Leopard
// Linux: kernel version 206 = 2.6
// Windows Phone 8.0: 800=8.0, 810=8.1(Blue)
int GetOperatingSystemNumeric();

std::string GetMacAddress();
#endif

#if UNITY_WIN
// Return true if specified mac address exists in the system
bool MacAddressExists(const std::string &macaddr);
#endif

#if WEBPLUG && UNITY_OSX
bool IsRunningInDashboardWidget();
#endif

#if UNITY_WINRT
int GetManagedMemorySize();
void Cleanup();
#endif

#if UNITY_LINUX
	std::string GetXDGCompliantPath (const std::string &environmentVariable, const std::string &homedirRelativePath);
#endif


RuntimePlatform GetRuntimePlatform();

	bool IsPlatformStandalone( RuntimePlatform p );
	bool IsPlatformWebPlayer( RuntimePlatform p );

	std::string GetRuntimePlatformString();
	std::string GetRuntimePlatformString( RuntimePlatform p );

	std::string GetPersistentDataPath();		/// A path for data that can be considered "long-lived", possibly to time of un-installation.
	std::string GetTemporaryCachePath();		/// A path for "short-term" data, that may out-live the running session.

	char const* GetDeviceUniqueIdentifier ();
	char const* GetDeviceName ();
	char const* GetDeviceModel ();
	char const* GetDeviceSystemName ();
	char const* GetDeviceSystemVersion ();

	bool IsHandheldPlatform ();

	bool SupportsAccelerometer ();

	bool SupportsLocationService ();

	bool SupportsVibration ();

	bool SupportsAudio ();

	DeviceType GetDeviceType ();
} // namespace

// ADD_NEW_PLATFORM_HERE
#if !UNITY_SYSTEMINFO_OVERRIDE_GETDEVICETYPE
inline DeviceType systeminfo::GetDeviceType()
{
#if UNITY_IPHONE || UNITY_TVOS || UNITY_ANDROID || UNITY_BB10 || UNITY_WP_8_1 || UNITY_TIZEN || UNITY_STV
	return kDeviceTypeHandheld;
#elif UNITY_EDITOR || UNITY_OSX || UNITY_WIN || UNITY_LINUX || UNITY_WEBGL
	return kDeviceTypeDesktop;
#elif UNITY_XENON || UNITY_XBOXONE || UNITY_PS3 || UNITY_PS4 || UNITY_PSP2
	return kDeviceTypeConsole;
#else
	#error Should never get here. Add your platform.
	return kDeviceTypeUnknown;
#endif
}
#endif // !UNITY_SYSTEMINFO_OVERRIDE_GETDEVICETYPE


#endif
