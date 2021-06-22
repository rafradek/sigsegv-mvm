/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_base_boss.cpp
 */


void CTFBaseBoss::Spawn()
{
	// TODO
}


bool CTFBaseBoss::IsRemovedOnReset() const
{
	return false;
}


void CTFBaseBoss::UpdateCollisionBounds()
{
}

int CTFBaseBoss::GetCurrencyValue()
{
	return 125;
}

void CTFBaseBoss::ModifyDamage(CTakeDamageInfo *info) const
{
}


void CTFBaseBoss::BossThink()
{
	this->SetNextThink(gpGlobals->curtime);
	
	// TODO: model stuff based on health/damage
	
	if (this->m_bEnabled) {
		this->Update();
		
		if (/* byte @ 0x9c8 */) {
			CUtlVector<CTFPlayer *> players;
			CollectPlayers(&players, TEAM_ANY, true, false);
			
			FOR_EACH_VEC(players, i) {
				this->ResolvePlayerCollision(players[i]);
			}
		}
	}
}

void CTFBaseBoss::ResolvePlayerCollision(CTFPlayer *player)
{
	Vector boss_mins;
	Vector boss_maxs;
	this->CollisionProp()->WorldSpaceAABB(&boss_mins, &boss_maxs);
	
	Vector player_mins;
	Vector player_maxs;
	player->CollisionProp()->WorldSpaceAABB(&player_mins, &player_maxs);
	
	Vector new_pos = player->GetAbsOrigin();
	
	if (IsBoxIntersectingBox(player_mins, player_maxs, boss_mins, boss_maxs)) {
		float dx = boss_maxs.x - player_mins.x;
		float dy = boss_maxs.y - player_mins.y;
		float dz = boss_maxs.z - player_mins.z;
		
		if (dy <= dx) {
			if (dy <= dz) {
				new_pos.y += dy + 5.0f;
			} else {
				new_pos.z += dz + 5.0f;
			}
		} else {
			if (dz <= dx) {
				new_pos.z += dz + 5.0f;
			} else {
				new_pos.x += dx + 5.0f;
			}
		}
		
		Ray_t ray;
		ray.Init(new_pos, new_pos, player->CollisionProp()->OBBMins(), player->CollisionProp()->OBBMaxs());
		
		// TODO: what is collision group 8?
		trace_t tr;
		UTIL_TraceRay(ray, MASK_PLAYERSOLID, player, 8, &tr);
		
		if (tr.fraction < 1.0f || tr.allsolid || tr.startsolid) {
			ray.Init(new_pos + Vector(0.0f, 0.0f, 32.0f), new_pos,
				player->CollisionProp()->OBBMins(), player->CollisionProp()->OBBMaxs());
			
			UTIL_TraceRay(ray, MASK_PLAYER_SOLID, player, 8, &tr);
			
			if (tr.startsolid) {
				CTakeDamageInfo dmginfo(this, this, 100000.0f, DMG_CRUSH);
				player->TakeDamage(dmginfo);
				return;
			}
			
			new_pos = tr.endpos;
		}
		
		player->SetAbsOrigin(new_pos);
	}
}


float CTFBaseBossLocomotion::GetStepHeight() const
{
	return 100.0f;
}

float CTFBaseBossLocomotion::GetMaxJumpHeight() const
{
	return 100.0f;
}

float CTFBaseBossLocomotion::GetRunSpeed() const
{
	return 100.0f;
}
