/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/tf/tf_item_powerup_bottle.h
 */


class CTFPowerupBottle : public CTFWearable
{
public:
	CTFPowerupBottle();
	virtual ~CTFPowerupBottle();
	
	virtual UNKNOWN Precache() override;
	
	virtual void ReapplyProvision() override;
	
	virtual UNKNOWN UnEquip(CBasePlayer *player) override;
	
	virtual UNKNOWN GetSkin() override;
	
	// TODO: non-virtual funcs
	
	bool Use();
	
private:
	enum PowerupType : int
	{
		EMPTY            = 0,
		CRITBOOST        = 1,
		UBERCHARGE       = 2,
		RECALL           = 3,
		REFILL_AMMO      = 4,
		BUILDING_UPGRADE = 5,
		
		/* ? */
		//RADIUS_STEALTH   = 6,
	};
	
	PowerupType GetPowerupType() const;
	// TODO
	
	CNetworkVar(bool,          m_bActive);      // +0x5c0
	CNetworkVar(unsigned char, m_usNumCharges); // +0x5c1
	IntervalTimer m_itCooldown;                 // +0x5c4
};
