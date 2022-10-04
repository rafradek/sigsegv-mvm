#ifndef _INCLUDE_SIGSEGV_STUB_STRINGS_H_
#define _INCLUDE_SIGSEGV_STUB_STRINGS_H_


#include "link/link.h"


using char_0x104 = const char[0x104];


extern GlobalThunk<const char *[4]> g_aTeamNames;
extern GlobalThunk<Color[4]>        g_aTeamColors;

extern GlobalThunk<const char *[11]> g_aClassNames;
extern GlobalThunk<const char *[13]> g_aPlayerClassNames;
extern GlobalThunk<const char *[13]> g_aPlayerClassNames_NonLocalized;
extern GlobalThunk<const char *[13]> g_aRawPlayerClassNames;
extern GlobalThunk<const char *[13]> g_aRawPlayerClassNamesShort;

extern GlobalThunk<const char *[]> g_aWeaponNames;

extern GlobalThunk<const char *[]> g_aConditionNames;

extern GlobalThunk<const char *[]> g_szLoadoutStrings;
extern GlobalThunk<const char *[]> g_szLoadoutStringsForDisplay;

extern GlobalThunk<char_0x104[10]> g_szBotModels;
extern GlobalThunk<char_0x104[10]> g_szBotBossModels;
extern GlobalThunk<char_0x104[10]> g_szPlayerRobotModels;
extern GlobalThunk<const char[]>   g_szBotBossSentryBusterModel;

extern GlobalThunk<const char *[4]> s_TankModel;
extern GlobalThunk<const char *[4]> s_TankModelRome;

extern GlobalThunk<char_0x104[10]> g_szRomePromoItems_Hat;
extern GlobalThunk<char_0x104[10]> g_szRomePromoItems_Misc;

extern GlobalThunk<const char *[]> g_pszMPConcepts_thunk;

inline int GetClassIndexFromString(const char *pClassName)
{
	for (int i = 1; i <= 10; ++i) {
		// compare first N characters to allow matching both "heavy" and "heavyweapons"
		int classnameLength = V_strlen(g_aPlayerClassNames_NonLocalized[i]);

		if (V_strlen(pClassName) < classnameLength)
			continue;

		if (!V_strnicmp(g_aPlayerClassNames_NonLocalized[i], pClassName, classnameLength)) {
			return i;
		}
	}

	return TF_CLASS_UNDEFINED;
}

#endif
