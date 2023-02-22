#include "util/misc.h"
#include "stub/baseentity.h"
#include "stub/baseanimating.h"

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

void EntityMatrix::InitFromEntity( CBaseEntity *pEntity, int iAttachment=0 )
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

void EntityMatrix::InitFromEntityLocal( CBaseEntity *entity )
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