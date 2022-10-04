#include "stub/strings.h"


GlobalThunk<const char *[4]> g_aTeamNames ("g_aTeamNames");
GlobalThunk<Color[4]>        g_aTeamColors("g_aTeamColors");

GlobalThunk<const char *[11]> g_aClassNames                   ("g_aClassNames");
GlobalThunk<const char *[13]> g_aPlayerClassNames             ("g_aPlayerClassNames");
GlobalThunk<const char *[13]> g_aPlayerClassNames_NonLocalized("g_aPlayerClassNames_NonLocalized");
GlobalThunk<const char *[13]> g_aRawPlayerClassNames          ("g_aRawPlayerClassNames");
GlobalThunk<const char *[13]> g_aRawPlayerClassNamesShort     ("g_aRawPlayerClassNamesShort");

GlobalThunk<const char *[]> g_aWeaponNames("g_aWeaponNames");

GlobalThunk<const char *[]> g_aConditionNames("g_aConditionNames");

GlobalThunk<const char *[]> g_szLoadoutStrings          ("g_szLoadoutStrings");
GlobalThunk<const char *[]> g_szLoadoutStringsForDisplay("g_szLoadoutStringsForDisplay");

GlobalThunk<char_0x104[10]> g_szBotModels               ("g_szBotModels");
GlobalThunk<char_0x104[10]> g_szBotBossModels           ("g_szBotBossModels");
GlobalThunk<char_0x104[10]> g_szPlayerRobotModels       ("g_szPlayerRobotModels");
GlobalThunk<const char[]>   g_szBotBossSentryBusterModel("g_szBotBossSentryBusterModel");

GlobalThunk<const char *[4]> s_TankModel    ("s_TankModel");
GlobalThunk<const char *[4]> s_TankModelRome("s_TankModelRome");

GlobalThunk<char_0x104[10]> g_szRomePromoItems_Hat ("g_szRomePromoItems_Hat");
GlobalThunk<char_0x104[10]> g_szRomePromoItems_Misc("g_szRomePromoItems_Misc");

GlobalThunk<const char *[]> g_pszMPConcepts_thunk("g_pszMPConcepts");