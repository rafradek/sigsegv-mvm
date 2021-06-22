/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_body.h
 */


class CTFBotBody : public PlayerBody
{
public:
	CTFBotBody(INextBot *nextbot);
	virtual ~CTFBotBody();
	
	virtual float GetHeadAimTrackingInterval() const override;
};
