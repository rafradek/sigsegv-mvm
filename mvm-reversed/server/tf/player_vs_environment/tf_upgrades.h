/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_upgrades.h
 */


// sizeof: 0x48
struct UpgradeAttribBlock_t
{
	// 00 char[0x44]
	// 44 dword -1, probably handle
};


// sizeof: 0x4ac
// "func_upgradestation"
class CUpgrades : public CBaseTrigger, public CGameEventListener
{
public:
	CUpgrades(/* TODO */);
	virtual ~CUpgrades();
	
	virtual void Spawn() override;
	
	virtual UNKNOWN EndTouch(CBaseEntity *ent) override;
	
	virtual UNKNOWN InputEnable(inputdata_t& data) override;
	virtual UNKNOWN InputDisable(inputdata_t& data) override;
	
	virtual void FireGameEvent(IGameEvent *event) override;
	
	UNKNOWN InputReset(inputdata_t& data);
	
	UNKNOWN UpgradeTouch(CBaseEntity *ent);
	
	UNKNOWN ApplyUpgradeToItem(CTFPlayer *player, CEconItemView *item, int i1, int i2, bool b1, bool b2);
	UNKNOWN GetUpgradeAttributeName(int i1);
	UNKNOWN PlayerPurchasingUpgrade(CTFPlayer *player, int i1, int i2, bool b1, bool b2, bool b3);
	
	UNKNOWN GrantOrRemoveAllUpgrades(CTFPlayer *player, bool b1, bool b2);
	// known to exist: CUpgrades::GrantAllUpgrades(CTFPlayer*, bool)
	// (see server.dylib 20140212a)
	
private:
	UNKNOWN ApplyUpgradeAttributeBlock(UpgradeAttribBlock_t *block, int i1, CTFPlayer *player, bool b1);
	UNKNOWN NotifyItemOnUpgrade(CTFPlayer *player, unsigned short i1, bool b1);
	UNKNOWN ReportUpgrade(CTFPlayer *player, int i1, int i2, int i3, int i4, bool b1, bool b2, bool b3);
	
	UNKNOWN RestoreItemAttributeToBaseValue(CEconItemAttributeDefinition *def, CEconItemView *item);
	UNKNOWN RestorePlayerAttributeToBaseValue(CEconItemAttributeDefinition *def, CTFPlayer *player);
	
	// TODO
};

// TODO: ApplyUpgrade_Default(const CMannVsMachineUpgrades& upgrade, CTFPlayer *player, CEconItemView *item, int i1, bool b1);
