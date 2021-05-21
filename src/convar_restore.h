#ifndef _INCLUDE_SIGSEGV_CONVAR_RESTORE_H_
#define _INCLUDE_SIGSEGV_CONVAR_RESTORE_H_


namespace ConVar_Restore
{
	void Register(ConCommandBase *pCommand);
	
	void Save();
	void SaveNonDefault();
	void Load();
	void Reset();

	void OnExtLoad();
	void OnExtUnload();
}


#endif
