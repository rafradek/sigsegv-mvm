/* handy utility commands */

#include "stub/gamerules.h"
#include "util/iterate.h"


CON_COMMAND_F(find_ent_ex, "Find and list all entities with classnames or targetnames that match the specified wildcard string.\nFormat: find_ent_ex <wildcard>", FCVAR_NONE)
{
	if (args.ArgC() < 2) {
		Msg("Format: find_ent_ex <wildcard>\n");
		return;
	}
	
	const char *pszWildcard = args[1];
	Msg("Searching for entities with class/target name matching wildcard string: '%s'\n", pszWildcard);
	
	size_t nMaxClass  = 0;
	size_t nMaxTarget = 0;
	std::map<int, CBaseEntity *> matches;
	
	size_t nTotalEntities = 0;
	
	ForEachEntity([&](CBaseEntity *pEntity){
		++nTotalEntities;
		
		if (pEntity->ClassMatches(pszWildcard) || pEntity->NameMatches(pszWildcard)) {
			matches.emplace(ENTINDEX(pEntity), pEntity);
			
			nMaxClass  = Max(nMaxClass,  2 + strlen(pEntity->GetClassname()));
			nMaxTarget = Max(nMaxTarget, 2 + strlen(STRING(pEntity->GetEntityName())));
		}
	});
	
	if (!matches.empty()) {
		nMaxClass  = Max(nMaxClass,  strlen("CLASSNAME"));
		nMaxTarget = Max(nMaxTarget, strlen("TARGETNAME"));
		
		Msg("   %-8s  %-*s  %-*s\n", "ENTINDEX", nMaxClass, "CLASSNAME", nMaxTarget, "TARGETNAME");
		
		for (const auto& pair : matches) {
			int entindex         = pair.first;
			CBaseEntity *pEntity = pair.second;
			
			Msg("   %-8s  %-*s  %-*s\n", (const char *)CFmtStr("#%d", entindex),
				nMaxClass,  (const char *)CFmtStr("'%s'", pEntity->GetClassname()),
				nMaxTarget, (const char *)CFmtStr("'%s'", STRING(pEntity->GetEntityName())));
		}
	}
	
	Msg("Found %u matches, out of %u total entities.\n", matches.size(), nTotalEntities);
}


CON_COMMAND_F(sig_setcustomupgradesfile, "Equivalent to firing the SetCustomUpgradesFile on a tf_gamerules entity, except it will always work", FCVAR_NONE)
{
	if (args.ArgC() < 2) {
		Msg("Format: sig_setcustomupgradesfile <path>\n");
		return;
	}
	
	if (TFGameRules() == nullptr) {
		Msg("sig_setcustomupgradesfile: TFGameRules() == nullptr\n");
		return;
	}
	
	TFGameRules()->SetCustomUpgradesFile(args[1]);
}
