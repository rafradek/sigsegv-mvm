#include "mod.h"


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
			for (auto& backtrace : this->m_Backtraces) {
				Msg("%s\n", backtrace->GetName());
			}
		}
		
		void AddBacktrace(const char *lib, const char *pattern)
		{
			auto backtrace = new CFuncBacktrace(LibMgr::Lib_FromString(lib), pattern);
			if (backtrace->Load()) {
				backtrace->Enable();
				this->m_Backtraces.emplace_back(backtrace);
			} else {
				delete backtrace;
			}
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
	
	
	CON_COMMAND(sig_debug_backtrace_list, "")
	{
		s_Mod.ListBacktraces();
	}
	
	CON_COMMAND(sig_debug_backtrace_add, "")
	{
		DevMsg("Add backtrace: lib '%s' pattern '%s'\n", args[1], args[2]);
		s_Mod.AddBacktrace(args[1], args[2]);
	}
	
	CON_COMMAND(sig_debug_backtrace_clear, "")
	{
		DevMsg("Clear backtraces\n");
		s_Mod.ClearBacktraces();
	}
}
