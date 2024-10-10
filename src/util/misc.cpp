#include "util/misc.h"
#include "stub/baseentity.h"
#include "stub/baseanimating.h"
#include "stub/baseplayer.h"
#include "stub/usermessages_sv.h"
#include "stub/trace.h"
#include "stub/gamerules.h"
#include <gamemovement.h>

template<typename LINE_FUNC>
static void HexDump_Internal(LINE_FUNC&& line_func, const void *ptr, size_t len, bool absolute)
{
	for (size_t i = 0; i < len; i += 0x10) {
		size_t n = (i + 0x10 < len ? 0x10 : len - i);
		assert(n > 0 && n <= 0x10);
		
		char buf[256];
		if (absolute) {
			V_snprintf(buf, sizeof(buf), "%08x:  %*s  %*s  %*s  %*s  |%*s|\n", (uintptr_t)ptr + i, 11, "", 11, "", 11, "", 11, "", n, "");
		} else {
			V_snprintf(buf, sizeof(buf), "%06x:  %*s  %*s  %*s  %*s  |%*s|\n", i, 11, "", 11, "", 11, "", 11, "", n, "");
		}
		
		for (size_t j = 0; j < n; ++j) {
			auto c = *(reinterpret_cast<const char *>(ptr) + i + j);
			
			auto l_nibble_to_hex_lower = [](char n){ return (n < 10 ? n + '0' : (n - 10) + 'a'); };
			auto l_nibble_to_hex_upper = [](char n){ return (n < 10 ? n + '0' : (n - 10) + 'A'); };
			
			int pos = (absolute ? 8 : 6) + (3 * j);
			     if (j <  4) pos += 2;
			else if (j <  8) pos += 3;
			else if (j < 12) pos += 4;
			else             pos += 5;
			
			buf[pos + 0] = l_nibble_to_hex_lower((c >> 4) & 0x0f);
			buf[pos + 1] = l_nibble_to_hex_lower((c >> 0) & 0x0f);
			
			if (!isprint(c)) c = '.';
			buf[(absolute ? 8 : 6) + 56 + j] = c;
		}
		
		line_func(buf);
	}
}

std::string HexDump(const void *ptr, size_t len, bool absolute)
{
	std::string str;
	HexDump_Internal([&str](const char *line){ str += line; }, ptr, len, absolute);
	return str;
}

void HexDumpToSpewFunc(void (*func)(const char *, ...), const void *ptr, size_t len, bool absolute)
{
	HexDump_Internal([=](const char *line){ (*func)("%s", line); }, ptr, len, absolute);
}

void SendConVarValue(int playernum, const char *convar, const char *value)
{
	char data[256];
	bf_write buffer(data, sizeof(data));
	buffer.WriteUBitLong(5, 6);
	buffer.WriteByte(1);
	buffer.WriteString(convar);
	buffer.WriteString(value);
	INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(playernum));
	if (netchan != nullptr) {
		netchan->SendData(buffer);
	}
}

void EntityMatrix::InitFromEntity(CBaseEntity *pEntity, int iAttachment)
{
		
	if ( !pEntity )
	{
		Identity();
		return;
	}

	// Get an attachment's matrix?
	if ( iAttachment != 0 )
	{
		CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
		if ( pAnimating && pAnimating->GetModelPtr() )
		{
			Vector vOrigin;
			QAngle vAngles;
			if ( pAnimating->GetAttachment( iAttachment, vOrigin, vAngles ) )
			{
				((VMatrix *)this)->SetupMatrixOrgAngles( vOrigin, vAngles );
				return;
			}
		}
	}

	((VMatrix *)this)->SetupMatrixOrgAngles( pEntity->GetAbsOrigin(), pEntity->GetAbsAngles() );
}

void EntityMatrix::InitFromEntityLocal(CBaseEntity *entity)
{
	if ( !entity || !entity->edict() )
	{
		Identity();
		return;
	}
	((VMatrix *)this)->SetupMatrixOrgAngles( entity->GetLocalOrigin(), entity->GetLocalAngles() );
}

std::vector<std::string> BreakStringsForMultiPrint(std::string string, size_t maxSize, char breakChar)
{
	std::vector<std::string> result;
	size_t pos = 0;
	size_t posBeginString = 0;
	while(true) {
		bool stop = false;
		while(true) {
			size_t nextpos = string.find(breakChar, pos);
			if (nextpos - posBeginString > maxSize) break;

			if (nextpos == std::string::npos) {
				stop = true;
				break;
			}
			pos = nextpos + 1;
		}
		if (pos == posBeginString) break;
		result.push_back(string.substr(posBeginString, pos - posBeginString));
		posBeginString = pos;
		if (stop) break;
	}
	return result;
}

void CancelClientMenu(CBasePlayer *player)
{
	menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player));
	CRecipientFilter filter;
	filter.AddRecipient(player);
	int msg_type = usermessages->LookupUserMessage("ShowMenu");
	bf_write *msg = engine->UserMessageBegin(&filter, msg_type);

	if (msg != nullptr) {
		msg->WriteShort(1);
		msg->WriteByte(0);
		msg->WriteByte(false);
		msg->WriteString("\n");

		engine->MessageEnd();
	}
	msg = engine->UserMessageBegin(&filter, msg_type);

	if (msg != nullptr) {
		msg->WriteShort(0);
		msg->WriteByte(0);
		msg->WriteByte(false);
		msg->WriteString(" ");

		engine->MessageEnd();
	}
}

#ifdef SE_IS_TF2
    constexpr int DEFAULT_MAX_PLAYERS = 100;
#else
	constexpr int DEFAULT_MAX_PLAYERS = 65;
#endif
int FixSlotCrashPre()
{
	static ConVarRef sv_visiblemaxplayers("sv_visiblemaxplayers");
	int oldsv_visiblemaxplayers = 256; 
	if (sv_visiblemaxplayers.GetInt() <= 0 && sv->GetMaxClients() > DEFAULT_MAX_PLAYERS) {
		oldsv_visiblemaxplayers = sv_visiblemaxplayers.GetInt();
		sv_visiblemaxplayers.SetValue(DEFAULT_MAX_PLAYERS);
	}
	else if (sv_visiblemaxplayers.GetInt() > DEFAULT_MAX_PLAYERS) {
		oldsv_visiblemaxplayers = sv_visiblemaxplayers.GetInt();
		sv_visiblemaxplayers.SetValue(DEFAULT_MAX_PLAYERS);
	}
	return oldsv_visiblemaxplayers;
}
void FixSlotCrashPost(int val)
{
	static ConVarRef sv_visiblemaxplayers("sv_visiblemaxplayers");
	if (val != 256) {
		sv_visiblemaxplayers.SetValue(val);
	}
}

void FireEyeTrace(trace_t &tr, CBaseEntity *entity, float range, int mask, int collisionGroup) {
	Vector start = entity->EyePosition();
	Vector end, forward;
	AngleVectors(entity->EyeAngles(), &forward);
	VectorMA(start, range, forward, end);
	UTIL_TraceLine(start, end, mask, entity, COLLISION_GROUP_NONE, &tr);
}

class CGameMovement2 : public CGameMovement
{
public:
	virtual CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );
};

bool ResolvePlayerStuck(CBasePlayer *player, float oldScale, float maxMove, bool enemyOnly) {
	auto movement = (CGameMovement2 *)g_pGameMovement;

	const Vector& vOrigin = player->GetAbsOrigin();
	const QAngle& qAngle = player->GetAbsAngles();
	const Vector& vHullMins = (player->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN) * player->GetModelScale();
	const Vector& vHullMaxs = (player->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX) * player->GetModelScale();

	trace_t result;
	
	CBasePlayer *prevMovementPlayer = movement->player;
	movement->player = player;
	auto handleHit = movement->TestPlayerPosition(vOrigin, COLLISION_GROUP_PLAYER_MOVEMENT, result);
	// am I stuck? try to resolve it
	if (result.DidHit() && !(enemyOnly && (result.DidHitWorld() || result.m_pEnt == nullptr || result.m_pEnt->MyCombatCharacterPointer() == nullptr || result.m_pEnt->GetTeamNumber() == player->GetTeamNumber())))
	{
		float flPlayerHeight = vHullMaxs.z - vHullMins.z;
		float flPlayerWidth = vHullMaxs.x - vHullMins.x;
		float flExtraHeight = 10;
		float flSpaceMove = oldScale * flPlayerWidth * maxMove;

		static Vector vTest[] =
		{
			Vector( flSpaceMove * 0.5f, flSpaceMove * 0.5f, flExtraHeight ),
			Vector( -flSpaceMove * 0.5f, -flSpaceMove * 0.5f, flExtraHeight ),
			Vector( -flSpaceMove * 0.5f, flSpaceMove * 0.5f, flExtraHeight ),
			Vector( flSpaceMove * 0.5f, -flSpaceMove * 0.5f, flExtraHeight ),
			Vector( flSpaceMove, 0, flExtraHeight ),
			Vector( -flSpaceMove, 0, flExtraHeight ),
			Vector( 0, flSpaceMove, flExtraHeight ),
			Vector( 0, -flSpaceMove, flExtraHeight ),
			Vector( 0, 0, flPlayerHeight * 0.5 + flExtraHeight ),
			Vector( 0, 0, -flPlayerHeight - flExtraHeight )
		};
		for ( int i=0; i<ARRAYSIZE( vTest ); ++i )
		{
			Vector vTestPos = vOrigin + vTest[i];
			handleHit = movement->TestPlayerPosition(vTestPos, COLLISION_GROUP_PLAYER_MOVEMENT, result);
			if (!result.DidHit() && !(enemyOnly && (result.DidHitWorld() || result.m_pEnt == nullptr || result.m_pEnt->MyCombatCharacterPointer() == nullptr || result.m_pEnt->GetTeamNumber() == player->GetTeamNumber())))
			{
				player->Teleport(&vTestPos, &qAngle, NULL);
				movement->player = prevMovementPlayer;
				return true;
			}
		}
		movement->player = prevMovementPlayer;
		return false;
	}
	movement->player = prevMovementPlayer;
	return true;
}

string_t currentMapNameFull;
string_t currentMapNameNoWorkshop;
string_t GetCurrentMapNoWorkshop() {
	string_t mapname = gpGlobals->mapname;
	if (mapname == NULL_STRING) return NULL_STRING;
	
	currentMapNameFull = mapname;
	const char *afterPrefix = StringAfterPrefixCaseSensitive(STRING(mapname), "workshop/");
	if (afterPrefix != nullptr) {
		const char *suffix = strstr(afterPrefix, ".ugc");
		if (afterPrefix != nullptr && suffix != nullptr) {
			std::string s(afterPrefix, suffix - afterPrefix);
			currentMapNameNoWorkshop = AllocPooledString(s.c_str());
			return currentMapNameNoWorkshop;
		}
	}
	currentMapNameNoWorkshop = mapname;
	
	return currentMapNameNoWorkshop;
}