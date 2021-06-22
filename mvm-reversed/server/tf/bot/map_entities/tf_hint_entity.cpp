/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_hint_entity.cpp
 */


IMPLEMENT_AUTO_LIST(ITFBotHintEntityAutoList);


CBaseTFBotHintEntity::CBaseTFBotHintEntity()
{
	this->m_bDisabled = false;
}

CBaseTFBotHintEntity::~CBaseTFBotHintEntity()
{
}


void CBaseTFBotHintEntity::InputEnable(inputdata_t& inputdata)
{
	this->m_bDisabled = false;
}

void CBaseTFBotHintEntity::InputDisable(inputdata_t& inputdata)
{
	this->m_bDisabled = true;
}


bool CBaseTFBotHintEntity::OwnerObjectFinishBuilding() const
{
	CBaseObject *obj = ToBaseObject(this->GetOwnerEntity());
	if (obj == nullptr) {
		return false;
	}
	
	return !obj->m_bBuilding;
}

bool CBaseTFBotHintEntity::OwnerObjectHasNoOwner() const
{
	CBaseObject *obj = ToBaseObject(this->GetOwnerEntity());
	if (obj == nullptr) {
		return false;
	}
	
	if (obj->GetBuilder() == nullptr) {
		return true;
	}
	
	if (!obj->GetBuilder()->IsPlayerClass(TF_CLASS_ENGINEER)) {
		// BUG: spelling, lack of newline
		Warning("Object has an owner that's not engineer.");
	}
	
	return false;
}
