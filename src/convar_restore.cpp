#include "convar_restore.h"
#include "mem/detour.h"
#include <boost/tokenizer.hpp>
#include "CommandBuffer.h"

class CEmptyConVar : public ConVar {};


namespace ConVar_Restore
{
	ConCommand ccmd_save("sig_cvar_save", &Save, "Save the values of this mod's ConVars");
	ConCommand ccmd_savenondefault("sig_cvar_save_non_default", &SaveNonDefault, "Save the values of this mod's ConVars, excluding default values");
	ConCommand ccmd_load("sig_cvar_load", &Load, "Load the previously saved ConVar values of this mod");
	ConCommand ccmd_reset("sig_cvar_reset", &Reset, "Reset ConVars to default values");
	ConVar cvar_autosave("sig_cvar_autosave", "0", FCVAR_NONE, "Should ConVar values automatically be saved on disk during mod unload");
	
	std::list<ConVar *>     s_ConVars;
	std::list<ConCommand *> s_ConCmds;
	
	
	void Register(ConCommandBase *pCommand)
	{
		/* ignore s_EmptyConVar */
		if (dynamic_cast<CEmptyConVar *>(pCommand) != nullptr) return;
		
		if (pCommand->IsCommand()) {
		//	DevMsg("ConVar_Restore::Register: cmd \"%s\" @ %08x\n", cmd->GetName(), (uintptr_t)cmd);
			s_ConCmds.push_front(static_cast<ConCommand *>(pCommand));
		} else {
		//	DevMsg("ConVar_Restore::Register: var \"%s\" @ %08x\n", var->GetName(), (uintptr_t)var);
			s_ConVars.push_front(static_cast<ConVar *>(pCommand));
		}
	}
	
	void SaveActual(bool no_default, const char *path)
	{
	//	DevMsg("ConVar_Restore::Save\n");
		
		// Use text buffer instead of keyvalues for comments to be automatically generated

		CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );

		// auto kv = new KeyValues("SigsegvConVars");
		// kv->UsesEscapeSequences(true);
		
		//file.PutString("SigsegvConVars\n{\n");

		file.PutString("\n//-------------\n//These are default values for convars added by sigsegv extension. Changed values should be added into sigsegv_convars.cfg file\n//-------------\n\n");
		for (auto var : s_ConVars) {
		//	DevMsg("  %s\n", var->GetName());
			
			// if (kv->FindKey(var->GetName()) != nullptr) {
			// 	Warning("ConVar_Restore::Save: ConVar \"%s\" is a duplicate\n", var->GetName());
			// 	continue;
			// }
			if (var->IsFlagSet(FCVAR_DEVELOPMENTONLY)) {
				continue;
			}
			if (var->IsFlagSet(FCVAR_NEVER_AS_STRING)) {
				Warning("ConVar_Restore::Save: ConVar \"%s\" has unsupported flag FCVAR_NEVER_AS_STRING\n", var->GetName());
				continue;
			}
			
			if (no_default && strcmp(var->GetString(), var->GetDefault()) == 0) {
		//		DevMsg("    default: skip\n");
				continue;
			}
			
			file.PutString(var->GetName());
			file.PutString("	\"");
			file.PutString(var->GetString());
			file.PutChar('"');

			if (var->GetHelpText() != nullptr && *(var->GetHelpText())) {
				
				std::string help(var->GetHelpText());
				boost::tokenizer<boost::char_separator<char>> tokens(help, boost::char_separator<char>("\n"));

				bool first = true;
				for (auto &token : tokens) {
					if (!first) {
						file.PutChar('\n');
					}
					first = false;
					file.PutString("		// ");
					file.PutString(token.c_str());
				}
			}

			file.PutChar('\n');
			// kv->SetString(var->GetName(), var->GetString());
			
		//	auto subkey = new KeyValues(var->GetName());
		//	subkey->SetString("value", var->GetString());
		//	kv->AddSubKey(subkey);
		}

		//file.PutChar('}');
		if (!filesystem->WriteFile(path,"GAME", file)) {
			Warning("ConVar_Restore::Save: Could not save Ke27020yValues to \"cfg/sigsegv_convars.cfg\".\n");
		}
		
		//kv->deleteThis();
	}

	void Save()
	{
		SaveActual(false, "cfg/sigsegv_convars.cfg");
	}

	void SaveNonDefault()
	{
		SaveActual(true, "cfg/sigsegv_convars.cfg");
	}
	
	void Load()
	{
		CCommandBuffer buf;
		
		CUtlBuffer file( 0, 0, CUtlBuffer::TEXT_BUFFER );
		if (filesystem->ReadFile("cfg/sigsegv_convars.cfg", "GAME", file)) {
			char line[1024];
			while(file.GetBytesRemaining() > 0) {
				file.EatWhiteSpace();
				file.GetLine(line, sizeof(line));
				char name[256];
				CUtlBuffer linebuf(line, sizeof(line), CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
				int read = linebuf.ParseToken(CCommand::DefaultBreakSet(), name, sizeof(name));
				if (read == -1) continue;
				linebuf.EatWhiteSpace();

				bool inQuote = false;
				int lenc = 0;
				int len = 0;
				char *value = line + linebuf.TellGet();
				for (char *pline = value; *pline != '\0'; pline++) {
					char c = *pline;
					if (c == '"') {
						inQuote = !inQuote;
					}
					if (c == '/' && !inQuote && pline[1] == '/') {
						break;
					}
					lenc++;
					if (!isspace(c)) len = lenc;
				}
				if (*value == '"') {
					value++;
					len--;
					if (value[len-1] == '"') {
						len--;
					}
				}
				value[len] = '\0';
				ConCommandBase *base = icvar->FindCommandBase(name);
				if (base == nullptr) {
					Warning("ConVar_Restore::Load: ConVar \"%s\" doesn't exist\n", name);
					continue;
				}
				if (base->IsCommand()) {
					Warning("ConVar_Restore::Load: ConVar \"%s\" is actually a ConCommand\n", name);
					continue;
				}
				if (base->IsFlagSet(FCVAR_NEVER_AS_STRING)) {
					Warning("ConVar_Restore::Load: ConVar \"%s\" has unsupported flag FCVAR_NEVER_AS_STRING\n", name);
					continue;
				}
				
				auto var = static_cast<ConVar *>(base);
				var->SetValue(value);
			}
		}
	}

	void Reset()
	{
		for (auto var : s_ConVars) {
			var->SetValue(var->GetDefault());
		}
		CDetouredFunc::CleanUp();
	}

	void OnExtLoad()
	{
		Load();

		if (!filesystem->FileExists("cfg/sigsegv_convars_default.cfg", "GAME")) {
			Msg("ConVar_Restore: Default ConVar values saved in cfg/sigsegv_convars_default.cfg\n");
			SaveActual(false, "cfg/sigsegv_convars_default.cfg");
		}
	}

	void OnExtUnload()
	{
		if (cvar_autosave.GetBool())
			SaveActual(false, "cfg/sigsegv_convars.cfg");
	}
}
