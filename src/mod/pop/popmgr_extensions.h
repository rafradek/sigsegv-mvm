#ifndef _INCLUDE_SIGSEGV_MOD_POP_POPMGR_EXTENSIONS_H_
#define _INCLUDE_SIGSEGV_MOD_POP_POPMGR_EXTENSIONS_H_

namespace Mod::Pop::PopMgr_Extensions {
    bool ExtendedUpgradesNoUndo();
    bool ExtendedUpgradesOnly();
    IBaseMenu *DisplayExtraLoadoutItemsClass(CTFPlayer *player, int class_index, bool autoHide);
    IBaseMenu *DisplayExtraLoadoutItems(CTFPlayer *player, bool autoHide);
    bool HasExtraLoadoutItems(int class_index);
    CTFItemDefinition *GetCustomWeaponItemDef(std::string name);
	bool AddCustomWeaponAttributes(std::string name, CEconItemView *view);
	const char *GetCustomWeaponNameOverride(const char *name);
	int GetEventPopfile();
	bool PopFileIsOverridingJoinTeamBlueConVarOn();
	void AwardExtraItem(CTFPlayer *player, std::string &name, bool equipNow);
	void StripExtraItem(CTFPlayer *player, std::string &name, bool removeNow);
	void ResetExtraItems(CTFPlayer *player);
	void ApplyOrClearRobotModel(CTFPlayer *player);
	int GetMaxSpectators();
	bool SpyNoSapUnownedBuildings();
	int GetMaxRobotLimit();
	void SaveStateInfoBetweenMissions();
	void RestoreStateInfoBetweenMissions();
	void DisableLoadoutSlotReplace(bool disable);
}
#endif