#include "mod.h"


namespace Mod::Perf::Filesystem_Optimize
{
	int base_path_len;
    DETOUR_DECL_STATIC(bool, findFileInDirCaseInsensitive, const char *file, char* output, size_t bufSize)
	{
		bool hasupper = false;
		int i = 0;
		for(const char *c = file; *c != '\0'; c++) {
			hasupper |= i++ > base_path_len && *c >= 'A' && *c <= 'Z';
		}
		if (!hasupper) {
			return false;
		}
		strncpy(output, file, bufSize);
		V_strlower(output + base_path_len);
		struct stat stats;
		return stat(output, &stats) == 0;
		//return DETOUR_STATIC_CALL(file, output, bufSize);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Filesystem_Optimize")
		{
			// Faster implementation of findFileInDirCaseInsensitive, instead of looking for any matching file with a different case, only look for files with lowercase letters instead
			MOD_ADD_DETOUR_STATIC(findFileInDirCaseInsensitive, "findFileInDirCaseInsensitive");
		}
		virtual bool OnLoad() override
		{
			base_path_len = filesystem->GetSearchPath( "BASE_PATH", true, nullptr, 0);
			return true;
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_filesystem_optimize", "0", FCVAR_NOTIFY,
		"Mod: alter case insensitive search of loose files so that it simply looks for all lowercase path instead",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}