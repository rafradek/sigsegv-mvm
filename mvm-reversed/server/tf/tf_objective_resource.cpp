/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_objective_resource.cpp
 */


CTFObjectiveResource::CTFObjectiveResource()
{
	// TODO
	
	this->m_iszTeleporterName = AllocPooledString("teleporter");
}

virtual CTFObjectiveResource::~CTFObjectiveResource()
{
}


void CTFObjectiveResource::Spawn()
{
	CBaseTeamObjectiveResource::Spawn();
}


void CTFObjectiveResource::IncrementMannVsMachineWaveClassCount(string_t name, unsigned int flags)
{
	/* this function only does string pointer comparison, not true string
	 * content comparison; be careful */
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames[i] &&
			(this->m_nMannVsMachineClassFlags[i] & flags) != 0) {
			if (++this->m_bMannVsMachineWaveClassCounts[i] <= 0) {
				this->m_bMannVsMachineWaveClassCounts[i] = 1;
			}
			return;
		}
	}
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames2[i] &&
			(this->m_nMannVsMachineClassFlags2[i] & flags) != 0) {
			if (++this->m_bMannVsMachineWaveClassCounts2[i] <= 0) {
				this->m_bMannVsMachineWaveClassCounts2[i] = 1;
			}
			return;
		}
	}
}

void CTFObjectiveResource::DecrementMannVsMachineWaveClassCount(string_t name, unsigned int flags)
{
	/* this function only does string pointer comparison, not true string
	 * content comparison; be careful */
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames[i] &&
			(this->m_nMannVsMachineClassFlags[i] & flags) != 0) {
			if (--this->m_bMannVsMachineWaveClassCounts[i] < 0) {
				this->m_bMannVsMachineWaveClassCounts[i] = 0;
				this->SetMannVsMachineWaveClassActive(name, false);
			}
			return;
		}
	}
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames2[i] &&
			(this->m_nMannVsMachineClassFlags2[i] & flags) != 0) {
			if (--this->m_bMannVsMachineWaveClassCounts2[i] < 0) {
				this->m_bMannVsMachineWaveClassCounts2[i] = 0;
				this->SetMannVsMachineWaveClassActive(name, false);
			}
			return;
		}
	}
}


void CTFObjectiveResource::IncrementTelporterCount()
{
	this->IncrementMannVsMachineWaveClassCount(this->m_iszTeleporterName,
		CLASSFLAG_MISSION);
}

void CTFObjectiveResource::DecrementTeleporterCount()
{
	this->DecrementMannVsMachineWaveClassCount(this->m_iszTeleporterName,
		CLASSFLAG_MISSION);
}


int CTFObjectiveResource::GetMannVsMachineWaveClassCount(int index)
{
	if (index < 12) {
		return this->m_nMannVsMachineWaveClassCounts[index];
	} else if (index < 24) {
		return this->m_nMannVsMachineWaveClassCounts2[index - 12];
	}
	
	return 0;
}

void CTFObjectiveResource::SetMannVsMachineWaveClassCount(int index, int count)
{
	if (index < 12) {
		this->m_nMannVsMachineWaveClassCounts[index] = count;
	} else if (index < 24) {
		this->m_nMannVsMachineWaveClassCounts2[index - 12] = count;
	}
}


void CTFObjectiveResource::ClearMannVsMachineWaveClassFlags()
{
	for (int i = 0; i < 12; ++i) {
		this->m_nMannVsMachineWaveClassFlags[i] = 0;
	}
	
	for (int i = 0; i < 12; ++i) {
		this->m_nMannVsMachineWaveClassFlags2[i] = 0;
	}
}

void CTFObjectiveResource::AddMannVsMachineWaveClassFlags(int index, unsigned int flags)
{
	if (index < 12) {
		this->m_nMannVsMachineWaveClassFlags[index] |= flags;
	} else if (index < 24) {
		this->m_nMannVsMachineWaveClassFlags2[index - 12] |= flags;
	}
}


void CTFObjectiveResource::SetMannVsMachineWaveClassName(int index, string_t name)
{
	if (index < 12) {
		this->m_iszMannVsMachineWaveClassNames[index] = name;
	} else if (index < 24) {
		this->m_iszMannVsMachineWaveClassNames2[index - 12] = name;
	}
}

void CTFObjectiveResource::SetMannVsMachineWaveClassActive(string_t name, bool active)
{
	/* this function only does string pointer comparison, not true string
	 * content comparison; be careful */
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames[i]) {
			this->m_bMannVsMachineWaveClassActive[i] = active;
			return;
		}
	}
	
	for (int i = 0; i < 12; ++i) {
		if (name == this->m_iszMannVsMachineWaveClassNames2[i]) {
			this->m_bMannVsMachineWaveClassActive2[i] = active;
			return;
		}
	}
}
