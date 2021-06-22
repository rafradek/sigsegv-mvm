/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_point_weapon_mimic.h
 */


class CTFPointWeaponMimic : public CPointEntity
{
public:
	CTFPointWeaponMimic();
	virtual ~CTFPointWeaponMimic();
	
	virtual void Spawn() override;
	
	float GetSpeed() const;
	QAngle GetFiringAngles() const;
	
	void Fire();
	void FireArrow();
	void FireGrenade();
	void FireRocket();
	void FireStickyGrenade();
	
	// entity inputs
	DetonateStickies
	InputFireOnce
	InputFireMultiple
	
private:
	int m_nWeaponType;                                               // +0x360
	// 364 
	const char *m_pzsFireSound;                                      // +0x368
	const char *m_pzsFireParticles;                                  // +0x36c
	const char *m_pzsModelOverride;                                  // +0x370
	float m_flModelScale;                                            // +0x374
	float m_flSpeedMin;                                              // +0x378
	float m_flSpeedMax;                                              // +0x37c
	float m_flDamage;                                                // +0x380
	float m_flSplashRadius;                                          // +0x384
	float m_flSpreadAngle;                                           // +0x388
	bool m_bCrits;                                                   // +0x38c
	CUtlVector<CHandle<CTFGrenadePipebombProjectile>> m_StickyBombs; // +0x390
};
