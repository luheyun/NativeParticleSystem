#include "PluginPrefix.h"
#include "SystemInfo.h"

// ADD_NEW_PLATFORM_HERE: review this whole file

namespace systeminfo {

const char* kUnsupportedIdentifier = "n/a";
static OperatingSystemFamily g_operatingSystemFamily = kUninitializedFamily;

RuntimePlatform GetRuntimePlatform()
{
		#if UNITY_OSX
			#if WEBPLUG
			if (systeminfo::IsRunningInDashboardWidget ())
				return kOSXDashboardPlayer;
			else
				return kOSXWebPlayer;
			#else
			return kOSXPlayer;
			#endif
		#elif UNITY_WIN && !UNITY_WINRT
			#if WEBPLUG
			return kWindowsWebPlayer;
			#else
			return kWindowsPlayer;
			#endif
		#elif UNITY_IPHONE
			return kiPhonePlayer;
		#elif UNITY_ANDROID
			return kAndroidPlayer;
		#elif UNITY_LINUX
			#if WEBPLUG
				return kLinuxWebPlayer;
			#else
				return kLinuxPlayer;
			#endif
		#else
			#error Unknown platform
		#endif
}

OperatingSystemFamily GetOperatingSystemFamily() {
  if (g_operatingSystemFamily == kUninitializedFamily) {
  #if UNITY_OSX
      g_operatingSystemFamily = kMacOSXFamily;
  #elif UNITY_WIN
      g_operatingSystemFamily = kWindowsFamily;
  #elif UNITY_LINUX
      g_operatingSystemFamily = kLinuxFamily;
  #else
      g_operatingSystemFamily = kOtherFamily;
  #endif
}
  return g_operatingSystemFamily;
}

bool IsPlatformStandalone( RuntimePlatform p ) {
	return p == kWindowsPlayer || p == kOSXPlayer || p == kLinuxPlayer;
}
bool IsPlatformWebPlayer( RuntimePlatform p ) {
	return p == kWindowsWebPlayer || p == kOSXWebPlayer || p == kOSXDashboardPlayer;
}

std::string GetRuntimePlatformString()
{
	return GetRuntimePlatformString( GetRuntimePlatform() );

}

std::string GetRuntimePlatformString( RuntimePlatform p )
{
	switch (p)
	{
	case kOSXEditor:          return "OSXEditor";
	case kOSXPlayer:          return "OSXPlayer";
	case kWindowsPlayer:      return "WindowsPlayer";
	case kOSXWebPlayer:       return "OSXWebPlayer";
	case kOSXDashboardPlayer: return "OSXDashboardPlayer";
	case kWindowsWebPlayer:   return "WindowsWebPlayer";
	case kWindowsEditor:      return "WindowsEditor";
	case kiPhonePlayer:       return "iPhonePlayer";
	case ktvOSPlayer:         return "tvOSPlayer";
	case kPS3Player:          return "PS3Player";
	case kXenonPlayer:        return "XenonPlayer";
	case WiiUPlayer:          return "WiiUPlayer";
	case kAndroidPlayer:      return "AndroidPlayer";
	case kLinuxPlayer:        return "LinuxPlayer";
	case kLinuxWebPlayer:     return "LinuxWebPlayer";
	case kLinuxEditor:        return "LinuxEditor";
	case kWebGLPlayer:        return "WebGL";
	case kMetroPlayerX86:     return "MetroPlayerX86";
	case kMetroPlayerX64:     return "MetroPlayerX64";
	case kMetroPlayerARM:     return "MetroPlayerARM";
	case kBlackBerryPlayer:   return "BlackBerryPlayer";
	case kTizenPlayer:        return "TizenPlayer";
	case kSamsungTVPlayer:    return "SamsungTVPlayer";
	default:
		#if !UNITY_EXTERNAL_TOOL
		#endif
		return "Unknown";
	}
}

bool IsHandheldPlatform ()
{
#if UNITY_IPHONE || UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN
	return true;
#else
	return false;
#endif
}

bool SupportsLocationService ()
{
#if !UNITY_METRO
	return IsHandheldPlatform ();
#else
	return true;
#endif
}

#if !(UNITY_ANDROID || UNITY_IPHONE || UNITY_TVOS || UNITY_METRO)
	bool SupportsVibration ()
	{
		return IsHandheldPlatform ();
	}
#endif


SystemLanguage ISOToSystemLanguage (std::string &langTag)
{
	// Compares the first 2 characters
	#define CASE_LANG(Tag, ReturnType) do { if(langTag.compare(0, 2, #Tag) == 0) return ReturnType; } while(0)

	CASE_LANG(af, SystemLanguageAfrikaans);
	CASE_LANG(ar, SystemLanguageArabic);
	CASE_LANG(eu, SystemLanguageBasque);
	CASE_LANG(be, SystemLanguageBelarusian);
	CASE_LANG(bg, SystemLanguageBulgarian);
	CASE_LANG(ca, SystemLanguageCatalan);

	if (langTag.find("zh") != std::string::npos)
	{
		if (langTag.find("hans") != std::string::npos)
			return SystemLanguageChineseSimplified;
		if (langTag.find("hant") != std::string::npos)
			return SystemLanguageChineseTraditional;
		return SystemLanguageChinese;
	}

	CASE_LANG(cs, SystemLanguageCzech);
	CASE_LANG(da, SystemLanguageDanish);
	CASE_LANG(nl, SystemLanguageDutch);
	CASE_LANG(en, SystemLanguageEnglish);
	CASE_LANG(et, SystemLanguageEstonian);
	CASE_LANG(fo, SystemLanguageFaroese);
	CASE_LANG(fi, SystemLanguageFinnish);
	CASE_LANG(fr, SystemLanguageFrench);
	CASE_LANG(de, SystemLanguageGerman);
	CASE_LANG(el, SystemLanguageGreek);
	CASE_LANG(he, SystemLanguageHebrew);
	CASE_LANG(hu, SystemLanguageHungarian);
	CASE_LANG(is, SystemLanguageIcelandic);
	CASE_LANG(id, SystemLanguageIndonesian);
	CASE_LANG(it, SystemLanguageItalian);
	CASE_LANG(ja, SystemLanguageJapanese);
	CASE_LANG(ko, SystemLanguageKorean);
	CASE_LANG(lv, SystemLanguageLatvian);
	CASE_LANG(lt, SystemLanguageLithuanian);
	CASE_LANG(no, SystemLanguageNorwegian);
	CASE_LANG(pl, SystemLanguagePolish);
	CASE_LANG(pt, SystemLanguagePortuguese);
	CASE_LANG(ro, SystemLanguageRomanian);
	CASE_LANG(ru, SystemLanguageRussian);
	CASE_LANG(sr, SystemLanguageSerboCroatian);
	CASE_LANG(sk, SystemLanguageSlovak);
	CASE_LANG(sl, SystemLanguageSlovenian);
	CASE_LANG(es, SystemLanguageSpanish);
	CASE_LANG(sv, SystemLanguageSwedish);
	CASE_LANG(th, SystemLanguageThai);
	CASE_LANG(tr, SystemLanguageTurkish);
	CASE_LANG(uk, SystemLanguageUkrainian);
	CASE_LANG(vi, SystemLanguageVietnamese);
	return SystemLanguageUnknown;
}

std::string GetSystemLanguageCulture(int language)
{
	switch(language) {
		case SystemLanguageAfrikaans:		return "af";
		case SystemLanguageArabic:			return "ar";
		case SystemLanguageBasque:			return "eu";
		case SystemLanguageBelarusian:		return "be";
		case SystemLanguageBulgarian:		return "bg";
		case SystemLanguageCatalan:			return "ca";
		case SystemLanguageChinese:			return "zh";
		case SystemLanguageSerboCroatian:	return "hr";
		case SystemLanguageCzech:			return "cs";
		case SystemLanguageDanish:			return "da";
		case SystemLanguageDutch:			return "nl";
		case SystemLanguageEnglish:			return "en";
		case SystemLanguageEstonian:		return "et";
		case SystemLanguageFaroese:			return "fo";
		case SystemLanguageFinnish:			return "fi";
		case SystemLanguageFrench:			return "fr";
		case SystemLanguageGerman:			return "de";
		case SystemLanguageGreek:			return "el";
		case SystemLanguageHebrew:			return "he";
		case SystemLanguageHungarian:		return "hu";
		case SystemLanguageIcelandic:		return "is";
		case SystemLanguageIndonesian:		return "id";
		case SystemLanguageItalian:			return "it";
		case SystemLanguageJapanese:		return "ja";
		case SystemLanguageKorean:			return "ko";
		case SystemLanguageLatvian:			return "lv";
		case SystemLanguageLithuanian:		return "lt";
		case SystemLanguageNorwegian:		return "no";
		case SystemLanguagePolish:			return "pl";
		case SystemLanguagePortuguese:		return "pt";
		case SystemLanguageRomanian:		return "ro";
		case SystemLanguageRussian:			return "ru";
		case SystemLanguageSlovak:			return "sk";
		case SystemLanguageSlovenian:		return "sl";
		case SystemLanguageSpanish:			return "es";
		case SystemLanguageSwedish:			return "sv";
		case SystemLanguageThai:			return "th";
		case SystemLanguageTurkish:			return "tr";
		case SystemLanguageUkrainian:		return "uk";
		case SystemLanguageVietnamese:		return "vi";
		default: return "";
	}
}

std::string GetSystemLanguageString(int language)
{
	switch(language) {
		case SystemLanguageAfrikaans:		return "Afrikaans";
		case SystemLanguageArabic:			return "Arabic";
		case SystemLanguageBasque:			return "Basque";
		case SystemLanguageBelarusian:		return "Belarusian";
		case SystemLanguageBulgarian:		return "Bulgarian";
		case SystemLanguageCatalan:			return "Catalan";
		case SystemLanguageChinese:			return "Chinese";
		case SystemLanguageSerboCroatian:	return "SerboCroatian";
		case SystemLanguageCzech:			return "Czech";
		case SystemLanguageDanish:			return "Danish";
		case SystemLanguageDutch:			return "Dutch";
		case SystemLanguageEnglish:			return "English";
		case SystemLanguageEstonian:		return "Estonian";
		case SystemLanguageFaroese:			return "Faroese";
		case SystemLanguageFinnish:			return "Finnish";
		case SystemLanguageFrench:			return "French";
		case SystemLanguageGerman:			return "German";
		case SystemLanguageGreek:			return "Greek";
		case SystemLanguageHebrew:			return "Hebrew";
		case SystemLanguageHungarian:		return "Hungarian";
		case SystemLanguageIcelandic:		return "Icelandic";
		case SystemLanguageIndonesian:		return "Indonesian";
		case SystemLanguageItalian:			return "Italian";
		case SystemLanguageJapanese:		return "Japanese";
		case SystemLanguageKorean:			return "Korean";
		case SystemLanguageLatvian:			return "Latvian";
		case SystemLanguageLithuanian:		return "Lithuanian";
		case SystemLanguageNorwegian:		return "Norwegian";
		case SystemLanguagePolish:			return "Polish";
		case SystemLanguagePortuguese:		return "Portuguese";
		case SystemLanguageRomanian:		return "Romanian";
		case SystemLanguageRussian:			return "Russian";
		case SystemLanguageSlovak:			return "Slovak";
		case SystemLanguageSlovenian:		return "Slovenian";
		case SystemLanguageSpanish:			return "Spanish";
		case SystemLanguageSwedish:			return "Swedish";
		case SystemLanguageThai:			return "Thai";
		case SystemLanguageTurkish:			return "Turkish";
		case SystemLanguageUkrainian:		return "Ukrainian";
		case SystemLanguageVietnamese:		return "Vietnamese";
		default: return "";
	}
}

} // namespace
