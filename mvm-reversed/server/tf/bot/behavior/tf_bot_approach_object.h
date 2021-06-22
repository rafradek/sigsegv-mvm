/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_approach_object.h
 * used in MvM: TODO
 */


// sizeof: 0x481c
class CTFBotApproachObject : public Action<CTFBot>
{
public:
	CTFBotApproachObject(CBaseEntity *object, float dist);
	virtual ~CTFBotApproachObject();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CBaseEntity> m_hObject;   // +0x0034
	float m_flDist;                   // +0x0038
	PathFollower m_PathFollower;      // +0x003c
	CountdownTimer m_ctRecomputePath; // +0x4810
};
