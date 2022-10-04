#include "mod.h"
#include "util/scope.h"


namespace Mod::Prof::Frame_Prof
{
    CCycleCount timespent;
    float prevTime = 0.0f;
	int bytesPrevReliable = 0;
	int bytesPrev = 0;
	int bytesPrevVoice = 0;
	bool prevOverflow = false;
	int maxsize = 0;
	int maxsizereliable= 0;
	int maxsizevoice= 0;
	DETOUR_DECL_STATIC(void, _Host_RunFrame, float dt)
	{
        CTimeAdder timer(&timespent);
		DETOUR_STATIC_CALL(_Host_RunFrame)(dt);
        timer.End();
        if (floor(gpGlobals->curtime/5) != floor(prevTime) ) {
            DevMsg("Frame time: %f\n", timespent.GetSeconds()/5);
            timespent.Init();
            prevTime = gpGlobals->curtime/5;
        }
		//Msg("ones: %d %d %d %d\n", bytesPrev, bytesPrevReliable, bytesPrevVoice, prevOverflow);
		bytesPrevReliable = 0;
		bytesPrev = 0;
		bytesPrevVoice = 0;
		prevOverflow = false;
	}
	DETOUR_DECL_MEMBER(int, CNetChan_SendDatagram, bf_write *bf)
	{
		auto chan = reinterpret_cast<INetChannel *>(this);
		bytesPrev = MAX(bytesPrev ,chan->GetNumBitsWritten(false) + (bf != nullptr ? bf->GetNumBitsWritten() : 0));
		bytesPrevReliable = MAX(bytesPrevReliable, chan->GetNumBitsWritten(true));
		prevOverflow |= chan->IsOverflowed();
		auto result = DETOUR_MEMBER_CALL(CNetChan_SendDatagram)(bf);
		return result;
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Frame_Prof")
		{
			MOD_ADD_DETOUR_STATIC(_Host_RunFrame,               "_Host_RunFrame");
			//MOD_ADD_DETOUR_MEMBER(CNetChan_SendDatagram,               "CNetChan::SendDatagram");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_frame_prof", "0", FCVAR_NOTIFY,
		"Mod: show frame time %",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}