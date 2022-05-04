#ifndef _INCLUDE_SIGSEGV_MOD_POP_POPMGR_EXTENSIONS_H_
#define _INCLUDE_SIGSEGV_MOD_POP_POPMGR_EXTENSIONS_H_

namespace Mod::Pop::PopMgr_Extensions {
    bool ExtendedUpgradesNoUndo();
    IBaseMenu *DisplayExtraLoadoutItemsClass(CTFPlayer *player, int class_index);
    IBaseMenu *DisplayExtraLoadoutItems(CTFPlayer *player);
    bool HasExtraLoadoutItems(int class_index);
    CTFItemDefinition *GetCustomWeaponItemDef(std::string name);
	bool AddCustomWeaponAttributes(std::string name, CEconItemView *view);
	const char *GetCustomWeaponNameOverride(const char *name);
	int GetEventPopfile();
	bool PopFileIsOverridingJoinTeamBlueConVarOn();
	void AwardExtraItem(CTFPlayer *player, std::string &name);
	void StripExtraItem(CTFPlayer *player, std::string &name);
	void ApplyOrClearRobotModel(CTFPlayer *player);
	int GetMaxSpectators();
}
#endif