/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_hint.h
 */


// sizeof: 0x36c
class CTFBotHint : public CBaseEntity
{
public:
	CTFBotHint();
	virtual ~CTFBotHint();
	
	virtual void Spawn() override;
	
	virtual void UpdateOnRemove() override;
	
	// TODO: InputEnable
	// TODO: InputDisable
	
	bool IsFor(CTFBot *bot) const;
	
private:
	void UpdateNavDecoration();
	
	int m_team;        // +0x360
	int m_hint;        // +0x364
	bool m_isDisabled; // +0x368
};
