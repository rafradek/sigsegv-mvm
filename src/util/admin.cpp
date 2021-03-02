#include "util/admin.h"
#include "stub/baseplayer.h"


static IGamePlayer *GetSMPlayer(CBasePlayer *player)
{
	if (player == nullptr) return nullptr;
	return playerhelpers->GetGamePlayer(player->edict());
}

int GetSMTargets(CBasePlayer *caller, const char *pattern, std::vector<CBasePlayer *> &vec, char *target_name, int target_name_size, int flags)
{
	if (pattern == nullptr) return 0;

	cell_t targets[34];
	cmd_target_info_t target_info;
	target_info.admin = ENTINDEX(caller);
	target_info.pattern = pattern;
	target_info.targets = targets;
	target_info.max_targets = 34;
	target_info.target_name = target_name;
	target_info.target_name_maxlength = target_name_size;
	target_info.flags = flags;

	playerhelpers->ProcessCommandTarget(&target_info);
	for (int i = 0; i < target_info.num_targets; i++) {
		vec.push_back(UTIL_PlayerByIndex(target_info.targets[i]));
	}

	return target_info.reason;
}

AdminId GetPlayerSMAdminID(CBasePlayer *player)
{
	IGamePlayer *smplayer = GetSMPlayer(player);
	
	/* unsure whether this is strictly necessary */
	(void)smplayer->RunAdminCacheChecks();
	
	return smplayer->GetAdminId();
}


bool PlayerIsSMAdmin(CBasePlayer *player)
{
	AdminId id = GetPlayerSMAdminID(player);
	if (id == INVALID_ADMIN_ID) return false;
	
	/* not 100% sure on whether we really want to require this or not */
//	if (adminsys->GetAdminFlags(id, Access_Effective) == ADMFLAG_NONE) return false;
	
	return true;
}

bool PlayerIsSMAdminOrBot(CBasePlayer *player)
{
	return (player->IsBot() || PlayerIsSMAdmin(player));
}


FlagBits GetPlayerSMAdminFlags(CBasePlayer *player)
{
	AdminId id = GetPlayerSMAdminID(player);
	if (id == INVALID_ADMIN_ID) return ADMFLAG_NONE;
	
	FlagBits flags = adminsys->GetAdminFlags(id, Access_Effective);
	if ((flags & ADMFLAG_ROOT) != 0) return ADMFLAG_ALL;
	
	return flags;
}


bool PlayerHasSMAdminFlag(CBasePlayer *player, AdminFlag flag)
{
	AdminId id = GetPlayerSMAdminID(player);
	if (id == INVALID_ADMIN_ID) return false;
	
	FlagBits flags = adminsys->GetAdminFlags(id, Access_Effective);
	if ((flags & ADMFLAG_ROOT) != 0) return true;
	
	return ((flags & (FlagBits)(1 << flag)) != 0);
}


bool PlayerHasSMAdminFlags_All(CBasePlayer *player, FlagBits flag_mask)
{
	AdminId id = GetPlayerSMAdminID(player);
	if (id == INVALID_ADMIN_ID) return false;
	
	FlagBits flags = adminsys->GetAdminFlags(id, Access_Effective);
	if ((flags & ADMFLAG_ROOT) != 0) return true;
	
	return ((flags & flag_mask) == flag_mask);
}

bool PlayerHasSMAdminFlags_Any(CBasePlayer *player, FlagBits flag_mask)
{
	AdminId id = GetPlayerSMAdminID(player);
	if (id == INVALID_ADMIN_ID) return false;
	
	FlagBits flags = adminsys->GetAdminFlags(id, Access_Effective);
	if ((flags & ADMFLAG_ROOT) != 0) return true;
	
	return ((flags & flag_mask) != 0);
}
