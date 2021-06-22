/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotComponentInterface.cpp
 */


INextBotComponent::INextBotComponent(INextBot *nextbot)
	: m_NextBot(nextbot)
{
	this->Reset();
	
	nextbot->RegisterComponent(this);
}

INextBotComponent::~INextBotComponent()
{
}


void INextBotComponent::Reset()
{
	this->m_Dword04 = 0.0f;
	this->m_flTickInterval = gpGlobals->interval_per_tick;
}

void INextBotComponent::Upkeep()
{
}

INextBot *INextBotComponent::GetBot() const
{
	return this->m_NextBot;
}
