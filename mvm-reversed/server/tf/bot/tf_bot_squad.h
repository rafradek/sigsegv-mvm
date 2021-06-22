/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_squad.h
 */


class CTFBotSquad : public INextBotEventResponder
{
public:
	CTFBotSquad();
	virtual ~CTFBotSquad();
	
	virtual INextBotEventResponder *FirstContainedResponder() const override;
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override;
	
	struct Iterator
	{
		CTFBot *bot; // +0x0
		int index;   // +0x4
	};
	
	void CollectMembers(CUtlVector<CTFBot *> *members) const;
	
	Iterator GetFirstMember() const;
	Iterator GetNextMember(const Iterator& it) const;
	
	int GetMemberCount() const;
	CTFBot *GetLeader() const;
	
	void Join(CTFBot *bot);
	void Leave(CTFBot *bot);
	
	float GetMaxSquadFormationError() const;
	float GetSlowestMemberIdealSpeed(bool include_leader) const;
	float GetSlowestMemberSpeed(bool include_leader) const;
	
	bool IsInFormation() const;
	bool ShouldSquadLeaderWaitForFormation() const;
	
private:
	void DisbandAndDeleteSquad();
	
	CUtlVector<CHandle<CTFBot>> m_Members; // +0x04
	CHandle<CTFBot> m_hLeader;             // +0x18
	float m_flFormationSize;               // +0x1c
	bool m_bShouldPreserveSquad;           // +0x20
};
