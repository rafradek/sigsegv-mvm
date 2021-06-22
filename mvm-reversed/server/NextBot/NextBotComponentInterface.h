/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotComponentInterface.h
 */


class INextBotComponent : public INextBotEventResponder
{
public:
	INextBotComponent(INextBot *nextbot);
	virtual ~INextBotComponent();
	
	virtual void Reset();
	virtual void Update() = 0;
	virtual void Upkeep();
	virtual INextBot *GetBot() const;
	
protected:
	float m_Dword04;                    // +0x04
	float m_flTickInterval;             // +0x08
	INextBot *m_NextBot;                // +0x0c
	INextBotComponent *m_NextComponent; // +0x10
};
