#include "mod.h"
#include "util/misc.h"
#include "util/scope.h"

#include <regex>
#include <boost/algorithm/string/find.hpp>


namespace Mod::Util::ConFilter
{
	class CConExcludeFilter
	{
	public:
		virtual ~CConExcludeFilter() = default;
		
		bool ShouldExclude(const std::string& line)
		{
			bool excluded = this->Exclude(line);
			
			++this->m_iLinesTotal;
			if (excluded) ++this->m_iLinesExcluded;
			
			return excluded;
		}
		
		const std::string& GetString() const { return this->m_String; }
		bool               GetICase()  const { return this->m_bICase; }
		
		int GetLinesTotal()    const { return this->m_iLinesTotal;    }
		int GetLinesExcluded() const { return this->m_iLinesExcluded; }
		
		virtual const char *GetType() const = 0;
		
	protected:
		CConExcludeFilter(const char *str, bool icase) : m_String(str), m_bICase(icase) {}
		
		virtual bool Exclude(const std::string& line) const = 0;
		
		const std::string m_String;
		const bool        m_bICase;
		
	private:
		int m_iLinesTotal    = 0;
		int m_iLinesExcluded = 0;
	};
	
	class CConExcludeFilterSubstr final : public CConExcludeFilter
	{
	public:
		CConExcludeFilterSubstr(const char *str, bool icase) :
			CConExcludeFilter(str, icase) {}
		
		virtual const char *GetType() const override { return "substr"; }
		
	private:
		virtual bool Exclude(const std::string& line) const override
		{
			if (this->GetICase()) {
				return !boost::ifind_first(line, this->GetString()).empty();
			} else {
				return !boost::find_first(line, this->GetString()).empty();
			}
		}
	};
	
	class CConExcludeFilterRegex final : public CConExcludeFilter
	{
	public:
		#warning NEED try/catch for std::regex ctor!
		// (need to figure out how exactly to do that given that we're doing member initialization here...)
		CConExcludeFilterRegex(const char *str, bool icase) :
			CConExcludeFilter(str, icase), m_Regex(str, std::regex::optimize | std::regex::ECMAScript | (icase ? std::regex::icase : 0)) {}
		
		virtual const char *GetType() const override { return "regex"; }
		
	private:
		virtual bool Exclude(const std::string& line) const override
		{
			return std::regex_search(line, this->m_Regex, std::regex_constants::match_any);
		}
		
		std::regex m_Regex;
	};
	
	
	std::vector<std::unique_ptr<CConExcludeFilter>> filters;
	
	
	RefCount rc_TempDisable;
	
	
	CON_COMMAND(sig_util_confilter_list, "Show info about all console filters")
	{
		SCOPED_INCREMENT(rc_TempDisable);
		
		Msg("  #TOTAL  #EXCLU  TYPE    CASE   STRING\n");
		
		for (auto& filter : filters) {
			Msg("  %6d  %6d  %-6s  %-5s  \"%s\"\n",
				filter->GetLinesTotal(), filter->GetLinesExcluded(),
				filter->GetType(), (filter->GetICase() ? "icase" : "case"),
				filter->GetString().c_str());
		}
		
		Msg("%zu console filter(s) total.\n", filters.size());
	}
	
	CON_COMMAND(sig_util_confilter_clear, "Remove all console filters")
	{
		SCOPED_INCREMENT(rc_TempDisable);
		
		size_t n_cleared = filters.size();
		filters.clear();
		Msg("Removed all %zu console filter(s).\n", n_cleared);
	}
	
	CON_COMMAND(sig_util_confilter_add, "Add a console filter: <substr|regex> <case|icase> \"string\"")
	{
		SCOPED_INCREMENT(rc_TempDisable);
		
		auto l_usage = [&]{
			Msg("Usage: %s <substr|regex> <case|icase> \"string\"\n"
				"  Filter type:      'substr' => substring match\n"
				"                    'regex'  => regular expression match\n"
				"  Case sensitivity: 'case'   => case sensitive\n"
				"                    'icase'  => case insensitive\n",
				args[0]);
		};
		
		if (args.ArgC() != 4) {
			Msg("Expected 3 arguments.\n");
			l_usage(); return;
		}
		
		const char *arg_type = args[1];
		const char *arg_case = args[2];
		const char *arg_str  = args[3];
		
		bool t_regex;
		if (FStrEq(arg_type, "substr")) {
			t_regex = false;
		} else if (FStrEq(arg_type, "regex")) {
			t_regex = true;
		} else {
			Msg("Invalid filter type parameter '%s'.\n", arg_type);
			l_usage(); return;
		}
		
		bool c_insensitive;
		if (FStrEq(arg_case, "case")) {
			c_insensitive = false;
		} else if (FStrEq(arg_case, "icase")) {
			c_insensitive = true;
		} else {
			Msg("Invalid case sensitivity parameter '%s'.\n", arg_case);
			l_usage(); return;
		}
		
		if (t_regex) {
			filters.emplace_back(new CConExcludeFilterRegex(arg_str, c_insensitive));
		} else {
			filters.emplace_back(new CConExcludeFilterSubstr(arg_str, c_insensitive));
		}
		
		Msg("Added case-%ssensitive %s console filter \"%s\".\n",
			(c_insensitive ? "in" : ""), (t_regex ? "regex" : "substring"), arg_str);
	}
	
	
	#warning __gcc_regcall detours considered harmful!
	// POTENTIAL ISSUE: stupid code may call spew funcs with only partial lines
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, SpewRetval_t, D__SpewMessage, SpewType_t spewType, const char *pGroupName, int nLevel, const Color *pColor, const char *pMsgFormat, va_list args)
	{
		static std::mutex s_Mutex;
		std::lock_guard<std::mutex> lock(s_Mutex);
		
		/* shared buffer, guarded by mutex */
		static char s_Line[8192];
		
		if (rc_TempDisable == 0) {
			V_vsprintf_safe(s_Line, pMsgFormat, args);
			
			bool exclude = false;
			for (auto& filter : filters) {
				if (filter->ShouldExclude(s_Line)) {
					exclude = true;
					break;
				}
			}
			
			if (exclude) {
				printf("EXCLUDED: |%s|\n", s_Line);
				return SPEW_CONTINUE;
			}
		}
		
		// it's slightly annoying that vsprintf has to get called in here, and then called yet again in the actual function.
		// hypothetically, we could use our pre-vsprintf'd buffer with "%s" and pass that to the actual function,
		// but this would require us to fabricate a va_list from scratch, which technically you aren't supposed to do.
		// a way around this would be to make a variadic intermediary function in which we make a va_list the normal way,
		// but it'd probably have to be a lambda, and there's a retarded amount of overhead associated with doing that,
		// in addition to the uncertainty that it'd even necessarily work correctly
		
	//	return [&](const char *fmt, ...){
	//		va_list va;
	//		va_start(va, fmt);
	//		SpewRetval_t ret = DETOUR_STATIC_CALL(spewType, pGroupName, nLevel, pColor, pMsgFormat, va);
	//		va_end(va);
	//		return ret;
	//	}("%s", s_Line);
		
		return DETOUR_STATIC_CALL(spewType, pGroupName, nLevel, pColor, pMsgFormat, args);
	}
		
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:ConFilter")
		{
			MOD_ADD_DETOUR_STATIC(D__SpewMessage, "_SpewMessage [internal]");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_confilter", "0", FCVAR_NOTIFY,
		"Utility: enable enhanced console message filtering system",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}




// enhanced con_filter functionality?
// - regex based
// - multiple include filters
// - multiple exclude filters
// - multiple highlight filters (with color option)

// MUST BE WINDOWS CLIENT COMPATIBLE
