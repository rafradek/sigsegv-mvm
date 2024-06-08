#include "mod.h"
#include "stub/baseentity.h"
#include "util/vgui.h"
#include "factory.h"


#if 0
// CLIENT DEFINITION!
class CNetMessage : public INetMessage
{
public:
	CNetMessage() {}
	virtual ~CNetMessage() {}
	
	virtual void SetNetChannel(INetChannel *netchan) override { this->m_NetChannel = netchan; }
	virtual void SetReliable
	
	virtual int GetGroup() const override { return INetChannelInfo::GENERIC; }
	virtual INetChannel *GetNetChannel() const override { return this->m_NetChannel; }
	
	virtual 
	
protected:
	bool m_bReliable = true;
	INetChannel *m_NetChannel = nullptr;
};

// ...
#endif


class Base_CmdKeyValues : public INetMessage
{
public:
	KeyValues *GetKeyValues() const  { return this->m_pKeyValues; }
	void SetKeyValues(KeyValues *kv) { this->m_pKeyValues = kv; }
	
private:
	bool m_bReliable;          // +0x04
	INetChannel *m_NetChannel; // +0x08
	KeyValues *m_pKeyValues;   // +0x0c
};
class SVC_CmdKeyValues : public Base_CmdKeyValues {};
class CLC_CmdKeyValues : public Base_CmdKeyValues {};


namespace Mod::VGUI::Test
{
	bool IsClient() { return (ClientFactory() != nullptr); }
	
	
	DETOUR_DECL_MEMBER(bool, SVC_CmdKeyValues_Process)
	{
		auto msg = reinterpret_cast<SVC_CmdKeyValues *>(this);
		
		KeyValues *kv = msg->GetKeyValues();
		if (kv != nullptr) {
			Msg("[CLIENT] Got SVC_CmdKeyValues with this content:\n");
			KeyValuesDumpAsDevMsg(kv, 0, 0);
		} else {
			Warning("[CLIENT] Got SVC_CmdKeyValues, but the KeyValues ptr is nullptr!\n");
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("VGUI:Test")
		{
			if (IsClient()) {
				// ideally we'd detour CClientState::ProcessCmdKeyValues, but
				// that turns out to be difficult to do on Windows
				MOD_ADD_DETOUR_MEMBER(SVC_CmdKeyValues_Process, "SVC_CmdKeyValues::Process");
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_vgui_test", "0", FCVAR_NOTIFY,
		"Mod: test custom VGUI elements",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_vgui_test_sv_kvtest, "")
	{
		KeyValues *kv = new KeyValues("CustomVGUI");
		kv->SetString("key1", "value1");
		
		KeyValues *sub = kv->FindKey("sub", true);
		sub->SetString("key2", "value2");
		
		Msg("[SERVER] Sending SVC_CmdKeyValues with this content:\n");
		KeyValuesDumpAsDevMsg(kv, 0, 0);
		
		for (int i = 0; i < sv->GetNumClients(); ++i) {
			IClient *client = sv->GetClient(i);
			if (client == nullptr) continue;
			
			edict_t *edict = INDEXENT(client->GetPlayerSlot() + 1);
			if (edict == nullptr) continue;
			
			Msg("[SERVER] Sending to client '%s'\n", client->GetClientName());
			engine->ClientCommandKeyValues(edict, kv);
		}
	}
	
	
	CON_COMMAND(sig_vgui_test_cl_list, "")
	{
		if (!IsClient()) return;
		
		CUtlVector<const char *> names;
		CBuildFactoryHelper::GetFactoryNames(names);
		
		Msg("VGUI factories available:\n");
		for (auto name : names) {
			Msg("%s\n", name);
		}
	}
	
	
	CON_COMMAND(sig_vgui_test_cl_create, "")
	{
		if (!IsClient()) return;
		
		CUtlVector<const char *> names;
		CBuildFactoryHelper::GetFactoryNames(names);
		
		Msg("VGUI factories available:\n");
		for (auto name : names) {
			Msg("%s\n", name);
		}
	}
	
	
#if 0
	CON_COMMAND(sig_vgui_test_delete, "")
	{
		if (!IsClient()) return;
		if (pMyFrame == nullptr) return;
		
		Msg("Will delete MyFrame\n");
		delete pMyFrame;
		
		// TODO: try this
	//	Msg("Will delete MyFrame\n");
	//	delete reinterpret_cast<vgui::Frame *>(pMyFrame);
		
		// TODO: try this
	//	Msg("Will delete MyFrame\n");
	//	g_pVGuiPanel->DeletePanel(pMyFrame->GetVPanel());
		
	//	Msg("Will delete MyFrame\n");
	//	pMyFrame->~MyFrame();
	//	::operator delete(pMyFrame);
	//	// (might want a reinterpret_cast version of this...?)
		
		// ALSO TRY: vcall ->MarkForDeletion()
		
		// last option:
		// manually call non-deleting dtor via thunk
		// then call ::operator delete
		
		pMyFrame = nullptr;
		
		Msg("Success\n");
	}
#endif
	
	
	
	
#if 0
	MyPanel *pMyPanel = nullptr;
	
	CON_COMMAND(sig_vgui_test_create, "")
	{
		if (!IsClient()) return;
		if (pMyPanel != nullptr) return;
		
		Msg("Will create MyPanel\n");
		pMyPanel = MyPanel::Create();
		if (pMyPanel == nullptr) {
			Msg("MyPanel::Create returned nullptr\n");
			return;
		}
		
	//	Msg("Will call GetClientModeNormal\n");
	//	IClientMode *pClientMode = GetClientModeNormal();
		
		Msg("g_pClientMode = %08x\n", (uintptr_t)g_pClientMode);
		for (int i = 0; i < 0x100; i += 4) {
			Msg("g_pClientMode->VT[%04x] = %08x\n", i, (*(uintptr_t **)(g_pClientMode))[i / 4]);
		}
		
		Msg("Will call IClientMode::GetViewport\n");
		vgui::Panel *pViewport = g_pClientMode->GetViewport();
		if (pViewport == nullptr) {
			Msg("IClientMode::GetViewport returned nullptr\n");
			return;
		}
		
		Msg("Will set parent to %08x\n", (uintptr_t)pViewport);
		pMyPanel->SetParent(pViewport);
		
		KeyValues *pSettings = new KeyValues("Settings");
		pSettings->SetString("fieldName", "MyPanel");
		pSettings->SetString("xpos",      "100");
		pSettings->SetString("ypos",      "10");
		pSettings->SetString("wide",      "20");
		pSettings->SetString("tall",      "200");
		pSettings->SetInt   ("visible",   1);
		pSettings->SetInt   ("enabled",   1);
		Msg("Will apply panel settings from KV\n");
		pMyPanel->ApplySettings(pSettings);
		pSettings->deleteThis();
		
	//	Msg("Will set name to \"MyPanel\"\n");
	//	pMyPanel->SetName("MyPanel");
	//	
	//	Msg("Will set pos to (10, 10)\n");
	//	pMyPanel->SetPos(10, 10);
	//	
	//	Msg("Will set size to (40, 30)\n");
	//	pMyPanel->SetSize(40, 30);
		
		
		
		Msg("Success\n");
	}
	
	CON_COMMAND(sig_vgui_test_delete, "")
	{
		if (!IsClient()) return;
		if (pMyPanel == nullptr) return;
		
		Msg("Will delete MyPanel\n");
		delete pMyPanel;
		
		pMyPanel = nullptr;
		
		Msg("Success\n");
	}
#endif
	
	
	
	
#if 0
	
#if defined _LINUX
	constexpr ptrdiff_t OFF_PANEL_DTOR_D1 = 0x007c;
	constexpr ptrdiff_t OFF_PANEL_DTOR_D0 = 0x0080;
	
	class FakePanel;
	void (FakePanel:: *real_D1)() = nullptr;
	void (FakePanel:: *real_D0)() = nullptr;
	
	class FakePanel
	{
	public:
		void dtor_D1()
		{
			Msg("Panel dtor D1 called! [this: %08x]\n", (uintptr_t)this);
			(this->*real_D1)();
		}
		
		void dtor_D0()
		{
			Msg("Panel dtor D0 called! [this: %08x]\n", (uintptr_t)this);
			(this->*real_D0)();
		}
	};
#endif
	
#if defined _WINDOWS
	constexpr ptrdiff_t OFF_PANEL_DTOR = 0x007c;
	
	class FakePanel;
	void (FakePanel:: *real_dtor)(bool) = nullptr;
	
	class FakePanel
	{
	public:
		void dtor(bool b1)
		{
			Msg("Panel dtor called! [this: %08x] [b1: %d]\n", (uintptr_t)this, b1);
			(this->*real_dtor)(b1);
		}
	};
#endif
	
	
	uintptr_t my_vtable[0x1000];
	
	
	vgui::Panel *pPanel = nullptr;
	vgui::VPANEL vpanel = 0;
	
	CON_COMMAND(sig_vgui_test_create, "")
	{
		if (!IsClient()) return;
		
		vgui::Panel *pPanel = CBuildFactoryHelper::InstancePanel("Panel");
		Msg("InstancePanel returned ptr: %08x\n", (uintptr_t)pPanel);
		if (pPanel == nullptr) return;
		
		vpanel = pPanel->GetVPanel();
		Msg("Created VPANEL: %08x\n", vpanel);
		
#if defined _WINDOWS
		uintptr_t **ppVT = reinterpret_cast<uintptr_t **>(pPanel);
		memcpy(my_vtable, *ppVT, sizeof(my_vtable));
		*ppVT = my_vtable;
		
		uintptr_t *p_dtor_entry = *ppVT + (OFF_PANEL_DTOR / 4);
		Msg("Panel dtor was: %08x\n", *p_dtor_entry);
		
		real_dtor = MakePtrToMemberFunc<FakePanel, void, bool>((void *)(*p_dtor_entry));
		*p_dtor_entry = (uintptr_t)GetAddrOfMemberFunc(&FakePanel::dtor);
		Msg("Panel dtor is now: %08x\n", *p_dtor_entry);
#endif
		
		
		
	}
	
	CON_COMMAND(sig_vgui_test_delete, "")
	{
		if (!IsClient()) return;
		
		Msg("About to delete VPANEL: %08x\n", vpanel);
		g_pVGuiPanel->DeletePanel(vpanel);
		Msg("Deleted panel\n");
	}
#endif
}
