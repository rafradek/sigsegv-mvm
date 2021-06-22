/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_use_teleporter.h
 * used in MvM: TODO
 */


// sizeof: 0x4820
class CTFBotUseTeleporter : public Action<CTFBot>
{
public:
	enum class UseHowType : int
	{
		// TODO
	};
	
	CTFBotUseTeleporter(CObjectTeleporter *teleporter, UseHowType how);
	virtual ~CTFBotUseTeleporter();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	bool IsTeleporterAvailable() const;
	
	CHandle<CObjectTeleporter> m_hTele; // +0x0034
	UseHowType m_How;                   // +0x0038
	// 003c PathFollower
	// 4810 CountdownTimer
	// 481c byte 0
};
