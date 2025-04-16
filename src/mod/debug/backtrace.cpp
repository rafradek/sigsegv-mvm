#include "mod.h"
#include "util/misc.h"


namespace Mod::Debug::Backtrace
{
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Backtrace")
		{
		}
		
		virtual void OnUnload() override
		{
			this->ClearBacktraces();
		}
		
		void ListBacktraces()
		{
			int idx   = 0;
			int width = NumDigits(this->m_Backtraces.size() - 1);
			
			Msg("Installed backtrace hooks: [%zu total]\n", this->m_Backtraces.size());
			for (auto& backtrace : this->m_Backtraces) {
				Msg("[%*d] 0x%08x %s\n", width, idx++, (uintptr_t)backtrace->GetFuncPtr(), backtrace->GetName());
			}
		}
		
		bool AddBacktrace(const char *lib, const char *pattern)
		{
			auto backtrace = new CFuncBacktrace(LibMgr::Lib_FromString(lib), pattern);
			if (backtrace->Load()) {
				backtrace->Enable();
				this->m_Backtraces.emplace_back(backtrace);
			} else {
				delete backtrace;
				return false;
			}
			return true;
		}
		
		bool DelBacktrace(int idx)
		{
			if (idx < 0 || (size_t)idx >= this->m_Backtraces.size()) return false;
			
			auto it = std::next(this->m_Backtraces.begin(), idx);
			(*it)->Unload();
			this->m_Backtraces.erase(it);
			
			return true;
		}
		
		void ClearBacktraces()
		{
			for (auto& backtrace : this->m_Backtraces) {
				backtrace->Unload();
			}
			this->m_Backtraces.clear();
		}
		
	private:
		std::list<std::unique_ptr<CFuncBacktrace>> m_Backtraces;
	};
	CMod s_Mod;
	
	
	CON_COMMAND(sig_debug_backtrace_list, "Debug: backtrace hooks: list installed backtraces")
	{
		s_Mod.ListBacktraces();
	}
	
	CON_COMMAND(sig_debug_backtrace_add, "Debug: backtrace hooks: install backtrace [args: <lib> <sym_regex>]")
	{
		if (args.ArgC() != 3) return;
		
		Msg("[Backtrace Add] Lib '%s', pattern '%s'\n", args[1], args[2]);
		bool success = s_Mod.AddBacktrace(args[1], args[2]);
		Msg("[Backtrace Add] %s.\n", (success ? "Success" : "Failure"));
	}
	
	CON_COMMAND(sig_debug_backtrace_del, "Debug: backtrace hooks: uninstall backtrace [args: <bt_num>]")
	{
		if (args.ArgC() != 2) return;
		
		Msg("[Backtrace Del] Number %d\n", V_atoi(args[1]));
		bool success = s_Mod.DelBacktrace(V_atoi(args[1]));
		Msg("[Backtrace Del] %s.\n", (success ? "Success" : "Failure"));
	}
	
	CON_COMMAND(sig_debug_backtrace_clear, "Debug: backtrace hooks: uninstall all backtraces")
	{
		Msg("[Backtrace Clear] Uninstalling all backtrace hooks.\n");
		s_Mod.ClearBacktraces();
	}
}
