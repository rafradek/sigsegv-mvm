/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_hint_entity.h
 */


DECLARE_AUTO_LIST(ITFBotHintEntityAutoList);

// sizeof: 0x36c
class CBaseTFBotHintEntity : public CPointEntity, public ITFBotHintEntityAutoList
{
public:
	enum HintType : int
	{
		TELEPORTER_EXIT = 0,
		SENTRY_GUN      = 1,
		ENGINEER_NEST   = 2,
	};
	
	CBaseTFBotHintEntity();
	virtual ~CBaseTFBotHintEntity();
	
	virtual HintType GetHintType() const = 0;
	
	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	
	bool OwnerObjectFinishBuilding() const;
	bool OwnerObjectHasNoOwner() const;
	
protected:
	bool m_bDisabled; // +0x364
	// 368 CHandle<T> (unused)
};
