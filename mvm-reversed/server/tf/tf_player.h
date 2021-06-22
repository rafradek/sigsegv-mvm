/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_player.h
 */


class CTFPlayertPathCost : public IPathCost
{
	// TODO
};


class CTFPlayer : public CBaseMultiplayerPlayer, public IHasAttributes,
	public IInventoryUpdateListener
{
public:
	// TODO
	
	bool IsMiniBoss() const;
	
	bool IsThreatAimingTowardMe(CBaseEntity *threat) const;
	bool IsThreatFiringAtMe(CBaseEntity *threat) const;
	
private:
	// TODO
	int m_nCurrency;          // +0x2838 NETPROP DT_TFLocalPlayerExclusive
	bool m_bIsMiniBoss;       // +0x283c NETPROP DT_TFPlayer
	// TODO
	bool m_bUseBossHealthBar; // +0x296c NETPROP DT_TFPlayer
	// TODO
};
