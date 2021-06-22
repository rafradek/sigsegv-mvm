/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBot.h
 */


class NextBotCombatCharacter : public CBaseCombatCharacter, public INextBot
{
public:
	virtual ~NextBotCombatCharacter();
	
	/* CBaseCombatCharacter overrides */
	// TODO
	
	/* INextBot overrides */
	virtual void OnNavAreaChanged(CNavArea *area1, CNavArea *area2) override;
	virtual CBaseCombatCharacter *GetEntity() const override;
	virtual NextBotCombatCharacter *GetNextBotCombatCharacter() const override;
	
	// TODO: non-virtual functions
	
protected:
	// TODO
	
private:
	// TODO
};


// TODO: NextBotDestroyer
