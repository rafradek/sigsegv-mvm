/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_squad.cpp
 */


CTFBotSquad::CTFBotSquad()
{
	this->m_flFormationSize = -1.0f;
	this->m_bShouldPreserveSquad = false;
}

CTFBotSquad::~CTFBotSquad()
{
}


INextBotEventResponder *CTFBotSquad::FirstContainedResponder() const
{
	if (this->m_Members.IsEmpty()) {
		return nullptr;
	}
	
	return this->m_Members[0]();
}

INextBotEventResponder *CTFBotSquad::NextContainedResponder(INextBotEventResponder *prev) const
{
	CHandle<CTFBot> hPrev;
	
	if (prev != nullptr) {
		CTFBot *bot_prev = static_cast<CTFBot *>(prev);
		if (bot_prev != nullptr) {
			hPrev = bot_prev;
		}
	}
	
	int idx = this->m_Members.Find(hPrev);
	if (idx != -1) {
		return this->m_Members[idx]();
	}
	
	return nullptr;
}


void CTFBotSquad::CollectMembers(CUtlVector<CTFBot *> *members) const
{
	FOR_EACH_VEC(this->m_Members, i) {
		CTFBot *bot = this->m_Members[i]();
		if (bot != nullptr && bot->IsAlive()) {
			members->AddToTail(this->m_Members[i]());
		}
	}
}


Iterator CTFBotSquad::GetFirstMember() const
{
	for (int i = 0; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i]();
		if (bot != nullptr && bot->IsAlive()) {
			return {
				.bot = this->m_Members[i](),
				.index = i,
			};
		}
	}
	
	return {
		.bot = nullptr,
		.index = -1,
	};
}

Iterator CTFBotSquad::GetNextMember(const Iterator& it) const
{
	for (int i = it.index + 1; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i]();
		if (bot != nullptr && bot->IsAlive()) {
			return {
				.bot = this->m_Members[i](),
				.index = i,
			};
		}
	}
	
	return {
		.bot = nullptr,
		.index = -1,
	};
}


int CTFBotSquad::GetMemberCount() const
{
	int count = 0;
	
	FOR_EACH_VEC(this->m_Members, i) {
		CTFBot *bot = this->m_Members[i]();
		if (bot != nullptr && bot->IsAlive()) {
			++count;
		}
	}
	
	return count;
}

CTFBot *CTFBotSquad::GetLeader() const
{
	return this->m_hLeader();
}


void CTFBotSquad::Join(CTFBot *bot)
{
	if (this->m_Members.IsEmpty()) {
		this->m_hLeader = bot;
	} else {
		if (TFGameRules() && TFGameRules()->IsMannVsMachineMode()) {
			bot->SetFlagTarget(nullptr);
		}
	}
	
	this->m_Members.AddToTail(bot);
}

void CTFBotSquad::Leave(CTFBot *bot)
{
	int idx = this->m_Members.Find(bot);
	if (idx != -1) {
		this->m_Members.Remove(idx);
	}
	
	CTFBot *leader = this->m_hLeader();
	if (bot == leader) {
		this->m_hLeader = nullptr;
		
		if (this->m_bShouldPreserveSquad) {
			CUtlVector<CTFBot *> members;
			this->CollectMembers(&members);
			
			if (!members.IsEmpty()) {
				this->m_hLeader = members[0];
			}
		}
	} else {
		if (TFGameRules() && TFGameRules()->IsMannVsMachineMode()) {
			CCaptureFlag *flag = bot->GetFlagToFetch();
			if (flag != nullptr) {
				bot->SetFlagTarget(flag);
			}
		}
	}
	
	if (this->GetMemberCount() == 0) {
		this->DisbandAndDeleteSquad();
	}
}


float CTFBotSquad::GetMaxSquadFormationError() const
{
	float error = 0.0f;
	
	/* exclude squad leader */
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *member = this->m_Members[i]();
		if (member == nullptr || !member->IsAlive()) {
			continue;
		}
		
		error = Max(error, this->m_Members[i]()->m_flFormationError);
	}
	
	return error;
}

float CTFBotSquad::GetSlowestMemberIdealSpeed(bool include_leader) const
{
	float speed = FLT_MAX;
	
	for (int i = (include_leader ? 0 : 1); i < this->m_Members.Count(); ++i) {
		CTFBot *member = this->m_Members[i]();
		if (member == nullptr || !member->IsAlive()) {
			continue;
		}
		
		speed = Min(speed, GetPlayerClassData(this->m_Members[i]()->
			GetPlayerClass()->GetClassIndex())->m_flMaxSpeed);
	}
	
	return speed;
}

float CTFBotSquad::GetSlowestMemberSpeed(bool include_leader) const
{
	float speed = FLT_MAX;
	
	for (int i = (include_leader ? 0 : 1); i < this->m_Members.Count(); ++i) {
		CTFBot *member = this->m_Members[i]();
		if (member == nullptr || !member->IsAlive()) {
			continue;
		}
		
		/* possible BUG: does CBasePlayer::MaxSpeed() actually factor in stuff
		 * like move speed attributes? I suspect not */
		speed = Min(speed, member->MaxSpeed());
	}
	
	return speed;
}


bool CTFBotSquad::IsInFormation() const
{
	/* exclude squad leader */
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *member = this->m_Members[i]();
		if (member == nullptr || !member->IsAlive()) {
			continue;
		}
		
		if (this->m_Members[i]()->m_bIsInFormation) {
			continue;
		}
		
		if (this->m_Members[i]()->GetLocomotionInterface()->IsStuck()) {
			continue;
		}
		
		if (this->m_Members[i]()->IsPlayerClass(TF_CLASS_MEDIC)) {
			continue;
		}
		
		if (this->m_Members[i]()->m_flFormationError > 0.75f) {
			return false;
		}
	}
	
	return true;
}

bool CTFBotSquad::ShouldSquadLeaderWaitForFormation() const
{
	/* exclude squad leader */
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *member = this->m_Members[i]();
		if (member == nullptr || !member->IsAlive()) {
			continue;
		}
		
		if (this->m_Members[i]()->m_flFormationError < 1.0f) {
			continue;
		}
		
		if (this->m_Members[i]()->m_bIsInFormation) {
			continue;
		}
		
		if (this->m_Members[i]()->GetLocomotionInterface()->IsStuck()) {
			continue;
		}
		
		if (!this->m_Members[i]()->IsPlayerClass(TF_CLASS_MEDIC)) {
			return true;
		}
	}
	
	return false;
}


void CTFBotSquad::DisbandAndDeleteSquad()
{
	FOR_EACH_VEC(this->m_Members, i) {
		CTFBot *bot = this->m_Members[i]();
		if (bot != nullptr) {
			bot->DeleteSquad();
		}
	}
	
	delete this;
}
