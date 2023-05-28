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
	virtual void GetScriptDesc() const;
	inline bool ComputeUpdateInterval() 
	{ 
		if ( m_Dword04 ) 
		{ 
			float interval = gpGlobals->curtime - m_Dword04;

			const float minInterval = 0.0001f;
			if ( interval > minInterval )
			{
				m_flTickInterval = interval;
				m_Dword04 = gpGlobals->curtime;
				return true;
			}

			return false;
		}

		// First update - assume a reasonable interval.
		// We need the very first update to do work, for cases
		// where the bot was just created and we need to propagate
		// an event to it immediately.
		m_flTickInterval = 0.033f;
		m_Dword04 = gpGlobals->curtime - m_flTickInterval;

		return true;
	}

	inline float GetUpdateInterval() 
	{ 
		return m_flTickInterval; 
	}

	inline void SetUpdateInterval(float interval) 
	{ 
		m_flTickInterval = interval; 
	}
protected:
	float m_Dword04;                    // +0x04
	float m_flTickInterval;             // +0x08
	INextBot *m_NextBot;                // +0x0c
	INextBotComponent *m_NextComponent; // +0x10
	uintptr_t pad; // +0x14
};
