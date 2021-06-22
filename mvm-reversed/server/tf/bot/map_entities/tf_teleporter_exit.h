/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_teleporter_exit.h
 */


// sizeof: 0x36c
class CTFBotHintTeleporterExit : public CBaseTFBotHintEntity
{
public:
	CTFBotHintTeleporterExit();
	virtual ~CTFBotHintTeleporterExit();
	
	virtual HintType GetHintType() const override;
};
