/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_locomotion.h
 */


class CTFBotLocomotion : public PlayerLocomotion
{
public:
	CTFBotLocomotion(INextBot *nextbot);
	virtual ~CTFBotLocomotion();
	
	virtual void Update() override;
	
	virtual void Approach(const Vector& v1, float f1) override;
	virtual void Jump() override;
	virtual float GetMaxJumpHeight() const override;
	virtual float GetDeathDropHeight() const override;
	virtual float GetRunSpeed() const override;
	virtual bool IsAreaTraversable(const CNavArea *area) const override;
	virtual bool IsEntityTraversable(CBaseEntity *ent, TraverseWhenType ttype) const override;
	virtual void AdjustPosture(const Vector& v1) override;
};
