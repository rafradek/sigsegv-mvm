/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_tank_boss.cpp
 */


ConCommand tf_mvm_tank_kill("tf_mvm_tank_kill", CMD_TankKill,
	"", FCVAR_CHEAT | FCVAR_GAMEDLL);
ConCommand tf_mvm_tank_health("tf_mvm_tank_health", CMD_TankHealth,
	"", FCVAR_CHEAT | FCVAR_GAMEDLL);


static const char *const s_TankModel[] = {
	"models/bots/boss_bot/boss_tank.mdl",
	"models/bots/boss_bot/boss_tank_damage1.mdl",
	"models/bots/boss_bot/boss_tank_damage2.mdl",
	"models/bots/boss_bot/boss_tank_damage3.mdl",
};

static const char *const s_TankModelRome[] = {
	"models/bots/tw2/boss_bot/boss_tank.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage1.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage2.mdl",
	"models/bots/tw2/boss_bot/boss_tank_damage3.mdl",
};


// TODO: server class init for CTFTankBoss ("tank_boss")
// TODO: server class init for CTFTankDestruction ("tank_destruction")


void CTFTankBoss::Spawn()
{
	if (!(TFGameRules() && TFGameRules()->IsMannVsMachineMode()) && this->m_initialHealth == 0) {
		if (this->GetHealth() > 0) {
			this->m_initialHealth = this->GetHealth();
		} else {
			this->m_initialHealth = 1000;
		}
	}
	
	CTFBaseBoss::Spawn();
	
	this->m_vecCollisionMins = vec3_origin;
	this->m_vecCollisionMaxs = vec3_origin;
	
	this->ChangeTeam(TF_TEAM_BLUE);
	
	this->m_iModelIndex = 0;
	this->SetModel("models/bots/boss_bot/boss_tank.mdl");
	
	this->SetModelIndexOverride(0, modelinfo->GetModelIndex(s_TankModel[this->m_iModelIndex]));
	this->SetModelIndexOverride(3, modelinfo->GetModelIndex(s_TankModelRome[this->m_iModelIndex]));
	
	this->m_iLastHealth = this->GetMaxHealth();
	
	this->AddGlowEffect();
	
	this->m_hTrackL = CreateEntityByName("prop_dynamic");
	if (this->m_hTrackL != nullptr) {
		this->m_hTrackL->SetModel("models/bots/boss_bot/tank_track_L.mdl");
		
		this->m_hTrackL->SetModelIndexOverride(0, modelinfo->GetModelIndex("models/bots/boss_bot/tank_track_L.mdl"));
		this->m_hTrackL->SetModelIndexOverride(3, modelinfo->GetModelIndex("models/bots/tw2/boss_bot/tank_track_L.mdl"));
		
		this->m_hTrackL->FollowEntity(this);
		
		int seq_fwd = this->m_hTrackL->LookupSequence("forward");
		if (seq_fwd != 0) {
			this->m_hTrackL->SetSequence(seq_fwd);
			
			this->m_hTrackL->SetPlaybackRate(1.0f);
			this->m_hTrackL->SetCycle(0.0f);
			
			this->m_hTrackL->ResetSequenceInfo();
		}
		
		this->m_vecTrackL = this->m_hTrackL->GetAbsOrigin();
	}
	
	this->m_hTrackR = CreateEntityByName("prop_dynamic");
	if (this->m_hTrackR != nullptr) {
		this->m_hTrackR->SetModel("models/bots/boss_bot/tank_track_R.mdl");
		
		this->m_hTrackR->SetModelIndexOverride(0, modelinfo->GetModelIndex("models/bots/boss_bot/tank_track_R.mdl"));
		this->m_hTrackR->SetModelIndexOverride(3, modelinfo->GetModelIndex("models/bots/tw2/boss_bot/tank_track_R.mdl"));
		
		this->m_hTrackR->FollowEntity(this);
		
		int seq_fwd = this->m_hTrackR->LookupSequence("forward");
		if (seq_fwd != 0) {
			this->m_hTrackR->SetSequence(seq_fwd);
			
			this->m_hTrackR->SetPlaybackRate(1.0f);
			this->m_hTrackR->SetCycle(0.0f);
			
			this->m_hTrackR->ResetSequenceInfo();
		}
		
		this->m_vecTrackR = this->m_hTrackR->GetAbsOrigin();
	}
	
	this->m_hBombMechanism = CreateEntityByName("prop_dynamic");
	if (this->m_hBombMechanism != nullptr) {
		this->m_hBombMechanism->SetModel("models/bots/boss_bot/bomb_mechanism.mdl");
		
		this->m_hBombMechanism->SetModelIndexOverride(0, modelinfo->GetModelIndex("models/bots/boss_bot/bomb_mechanism.mdl"));
		this->m_hBombMechanism->SetModelIndexOverride(3, modelinfo->GetModelIndex("models/bots/boss_bot/bomb_mechanism.mdl"));
		
		this->m_hBombMechanism->FollowEntity(this);
	}
	
	this->GetBodyInterface()->StartSequence("movement");
	
	this->m_iSmokeAttachment = this->LookupAttachment("smoke_attachment");
	
	if (this->m_hCurrentNode != nullptr) {
		this->SetAbsOrigin(this->m_hCurrentNode->WorldSpaceCenter());
	} else {
		this->m_hCurrentNode = dynamic_cast<CPathTrack *>(gEntList.FindEntityByClassname(nullptr, "path_track"));
		if (this->m_hCurrentNode != nullptr) {
			while (this->m_hCurrentNode->GetPrevious() != nullptr) {
				this->m_hCurrentNode = this->m_hCurrentNode->GetPrevious();
			}
			
			this->SetAbsOrigin(this->m_hCurrentNode->WorldSpaceCenter());
		}
	}
	
	this->m_flTotalDistance = 0.0f;
	
	this->m_NodeDists.AddToTail(this->m_flTotalDistance);
	
	this->m_iCurrentNode = 0;
	
	// 9d4 = m_hCurrentNode
	// 9d8 = m_hCurrentNode
	
	if (this->m_hCurrentNode != nullptr) {
		CPathTrack *node = this->m_hCurrentNode->GetNext();
		
		if (node != nullptr) {
			Vector dir = (node->GetAbsOrigin() - this->m_hCurrentNode->GetAbsOrigin());
			QAngle ang;
			VectorAngles(dir, ang);
			this->SetAbsAngles(ang);
			
			CPathTrack *prev = this->m_hCurrentNode;
			do {
				Vector2D delta = (node->GetAbsOrigin() - prev->GetAbsOrigin()).AsVector2D();
				
				this->m_flTotalDistance += delta.Length();
				this->m_NodeDists.AddToTail(this->m_flTotalDistance);
				
				prev = node;
				node = node->GetNext();
			} while (node != nullptr);
		}
	}
	
	this->SetBloodColor(DONT_BLEED);
	
	// eac = gpGlobals->curtime
	
	CBroadcastRecipientFilter filter;
	CBaseEntity::EmitSound(filter, ENTINDEX(this), "MVM.TankEngineLoop");
	this->EmitSound("MVM.TankStart");
	
	// TODO: tank alert announcement stuff
	
	// byte @ a00 = 0
	// a04 = 0
	// 9fc = curtime
	
	SetThink(&CTFTankBoss::TankBossThink);
	this->SetNextThink(gpGlobals->curtime);
}


void CTFTankBoss::UpdateCollisionBounds()
{
	if (this->m_vecCollisionMins.IsZero() || this->m_vecCollisionMaxs.IsZero()) {
		this->m_vecCollisionMins = this->CollisionProp()->OBBMins();
		this->m_vecCollisionMaxs = this->CollisionProp()->OBBMaxs();
	}
	
	/* 1.00 at 90 degree angles,
	 * 0.60 at 45 degree angles,
	 * has sin curve shape in between */
	float scale = 1.0f - (0.4f * abs(sin(DEG2RAD(this->GetAbsAngles().y))));
	
	Vector old_mins = this->m_vecCollisionMins;
	Vector old_maxs = this->m_vecCollisionMaxs;
	
	old_mins.x *= scale;
	old_mins.y *= scale;
	
	old_maxs.x *= scale;
	old_maxs.y *= scale;
	
	VMatrix mat;
	MatrixFromAngles(this->GetAbsAngles(), mat);
	
	Vector new_mins;
	Vector new_maxs;
	TransformAABB(mat, old_mins, old_maxs, new_mins, new_maxs);
	
	this->CollisionProp()->SetCollisionBounds(new_mins, new_maxs);
}

int CTFTankBoss::GetCurrencyValue()
{
	// TODO: double check this logic
	
	if (this->m_hCurrentNode == nullptr && !/* byte @ 0xa0d */) {
		return 0;
	}
	
	if (this->m_pWaveSpawn != nullptr) {
		return this->m_pWaveSpawn->GetCurrencyAmountPerDeath();
	} else {
		return 125;
	}
}

void CTFTankBoss::ModifyDamage(CTakeDamageInfo *info) const
{
	auto weapon = dynamic_cast<CTFWeaponBase *>(info->GetWeapon());
	if (weapon != nullptr && weapon->GetWeaponID() == TF_WEAPON_MINIGUN) {
		info->ScaleDamage(0.25f);
	}
}


void CTFTankBoss::TankBossThink()
{
	if (this->m_iLastHealth != this->GetHealth()) {
		this->m_iLastHealth = this->GetHealth();
		
		// TODO: calculation of new_idx based on damage percentage
		
		if (this->m_iModelIndex != new_idx) {
			const char *seq_name = this->GetSequenceName(this->GetSequence());
			float cycle = this->GetCycle();
			
			this->m_iModelIndex = new_idx;
			
			this->SetModel(s_TankModel[new_idx]);
			
			this->SetModelIndexOverride(0, modelinfo->GetModelIndex(s_TankModel[this->m_iModelIndex]));
			this->SetModelIndexOverride(3, modelinfo->GetModelIndex(s_TankModelRome[this->m_iModelIndex]));
			
			int seq = this->LookupSequence(seq_name);
			if (seq <= 0) {
				this->GetBodyInterface()->StartSequence("movement");
			} else {
				this->SetSequence(seq);
				this->SetPlaybackRate(1.0f);
				this->ResetSequenceInfo();
				this->SetCycle(cycle);
			}
		}
	}
	
	Vector fwd;
	Vector rt;
	Vector up;
	this->GetVectors(&fwd, &rt, &up);
	
	if (this->m_hTrackL != nullptr) {
		Vector v = this->GetAbsOrigin() - (rt * (180.0f / M_PI));
		
		float speed = (v - this->m_vecTrackL).Length() / gpGlobals->frametime;
		if (speed >= 80.0f) {
			this->m_hTrackL->SetPlaybackRate(1.0f);
		} else {
			this->m_hTrackL->SetPlaybackRate(speed / 80.0f);
		}
		
		this->m_vecTrackL = v;
	}
	
	if (this->m_hTrackR != nullptr) {
		Vector v = this->GetAbsOrigin() + (rt * (180.0f / M_PI));
		
		float speed = (v - this->m_vecTrackR).Length() / gpGlobals->frametime;
		if (speed >= 80.0f) {
			this->m_hTrackR->SetPlaybackRate(1.0f);
		} else {
			this->m_hTrackR->SetPlaybackRate(speed / 80.0f);
		}
		
		this->m_vecTrackR = v;
	}
	
	if (this->m_hCurrentNode != nullptr) {
		// TODO: wsc and vector stuff (sqrt, min, etc)
		
		if (TFGameRules() != nullptr) {
			// TODO
		}
		
		// TODO
	}
	
	// L_28
	
	if (/* byte @ 0xa00 */ && this->IsSequenceFinished()) {
		this->FirePopFileEvent(&this->m_OnBombDroppedOutput);
		
		// a00 = false
		
		TFGameRules()->BroadcastSound(255, "Announcer.MVM_Tank_Planted");
	}
	
	if (this->m_iSmokeAttachment > 0) {
		Vector attach;
		this->GetAttachment(this->m_iSmokeAttachment, attach);
		
		trace_t tr;
		UTIL_TraceLine(attach, attach + Vector(0.0f, 0.0f, 300.0f),
			MASK_SOLID_BRUSHONLY, this, 0, &tr);
		
		if (tr.fraction < 1.0f || tr.allsolid || tr.startsolid) {
			if (/* byte @ 0xa0c */) {
				StopParticleEffects(this);
				// 0xa0c = false
			}
		} else {
			if (!/* byte @ 0xa0c */) {
				DispatchParticleEffect("smoke_train", PATTACH_POINT_FOLLOW,
					this, this->m_iSmokeAttachment);
				// 0xa0c = true
			}
		}
	}
	
	if (this->m_ctCollisionCheck.IsElapsed()) {
		this->m_ctCollisionCheck.Start(0.5f);
		
		CBaseEntity *ents[0x40];
		int count = UTIL_EntitiesInBox(ents, 0x40,
			this->CollisionProp()->OBBMins() + this->GetAbsOrigin(),
			this->CollisionProp()->OBBMaxs() + this->GetAbsOrigin(),
			(FL_FAKECLIENT | FL_KILLME));
		
		for (int i = 0; i != count; ++i) {
			if (ents[i] == nullptr) continue;
			
			CTakeDamageInfo dmginfo(this, this, 4 * Max(ents[i]->GetHealth(), ents[i]->GetMaxHealth()), DMG_GENERIC);
			ents[i]->TakeDamage(dmginfo);
		}
	}
	
	this->UpdatePingSound();
	
	CTFBaseBoss::BossThink();
}


void CMD_TankKill(const CCommand& args)
{
	CBasePlayer *player = UTIL_GetCommandClient();
	if (player != nullptr) {
		CBaseEntity *tank = nullptr;
		while ((tank = gEntList.FindEntityByClassname(tank, "tank_boss")) != nullptr) {
			CTakeDamageInfo dmginfo(player, player, 10000000.0f, DMG_GENERIC);
			tank->TakeDamage(dmginfo);
		}
	}
}

void CMD_TankHealth(const CCommand& args)
{
	if (UTIL_GetCommandClient() == nullptr) {
		return;
	}
	
	if (args.ArgC() < 2) {
		Msg("Usage: %s <health to set all active tanks to>\n", args[0]);
		return;
	}
	
	// TODO
}
