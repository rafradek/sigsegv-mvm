#include "mod.h"
#include "util/backtrace.h"

#include <choreoscene.h>


struct EventNameMap_t
{
	CChoreoEvent::EVENTTYPE type;
	char const				*name;
};

static EventNameMap_t g_NameMap[] =
{
	{ CChoreoEvent::UNSPECIFIED,		"unspecified" },  // error condition!!!
	{ CChoreoEvent::SECTION,			"section" },
	{ CChoreoEvent::EXPRESSION,			"expression" },
	{ CChoreoEvent::LOOKAT,				"lookat" },
	{ CChoreoEvent::MOVETO,				"moveto" },
	{ CChoreoEvent::SPEAK,				"speak" },
	{ CChoreoEvent::GESTURE,			"gesture" },
	{ CChoreoEvent::SEQUENCE,			"sequence" },
	{ CChoreoEvent::FACE,				"face" },
	{ CChoreoEvent::FIRETRIGGER,		"firetrigger" },
	{ CChoreoEvent::FLEXANIMATION,		"flexanimation" },
	{ CChoreoEvent::SUBSCENE,			"subscene" },
	{ CChoreoEvent::LOOP,				"loop" },
	{ CChoreoEvent::INTERRUPT,			"interrupt" },
	{ CChoreoEvent::STOPPOINT,			"stoppoint" },
	{ CChoreoEvent::PERMIT_RESPONSES,	"permitresponses" },
	{ CChoreoEvent::GENERIC,			"generic" },
};

CChoreoEvent::EVENTTYPE CChoreoEvent::GetType( void )
{
	return (EVENTTYPE)m_fType;
}

const char *CChoreoEvent::GetName( void )
{
	return m_Name.Get();
}

const char *CChoreoEvent::NameForType( EVENTTYPE type )
{
	int i = (int)type;
	if ( i < 0 || i >= NUM_TYPES )
	{
		Assert( "!CChoreoEvent::NameForType:  bogus type!" );
		// returns "unspecified!!!";
		return g_NameMap[ 0 ].name;
	}

	return g_NameMap[ i ].name;
}


namespace Mod::Debug::UserMsg_Overflow_v2
{
	ConVar cvar_backtrace("sig_debug_usermsg_overflow_v2_backtrace", "0", FCVAR_NOTIFY,
		"Debug: enable backtraces from SendNetMsg");
	
	
	// CNetChan
	// ::m_StreamReliable     @ +0x20
	// ::m_ReliableDataBuffer @ +0x38
	
	bf_write& CNetChan_GetReliableStream(INetChannel *chan)
	{
		return *reinterpret_cast<bf_write *>((uintptr_t)chan + 0x20);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CNetChan_SendNetMsg, INetMessage& msg, bool bForceReliable, bool bVoice)
	{
		auto chan = reinterpret_cast<INetChannel *>(this);
	//	bf_write& m_StreamReliable = CNetChan_GetReliableStream(chan);
		
		int bits_before = chan->GetNumBitsWritten(true);
		auto result = DETOUR_MEMBER_CALL(msg, bForceReliable, bVoice);
		int bits_after = chan->GetNumBitsWritten(true);
		
		int bits_delta = (bits_after - bits_before);
		
		DevMsg("[TICK %7d] SendNetMsg[%s]: [before:%8d] [after:%8d] [delta:%8d] %s\n",
			gpGlobals->tickcount, chan->GetAddress(),
			bits_before, bits_after, bits_delta,
			/*msg.GetName(),*/ msg.ToString());
		
		if (cvar_backtrace.GetBool()) {
			BACKTRACE();
		}
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(float, CTFPlayer_PlayTauntRemapInputScene)
	{
		auto result = DETOUR_MEMBER_CALL();
		DevMsg("CTFPlayer::PlayTauntRemapInputScene returns %f\n", result);
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CBaseFlex_AddSceneEvent, CChoreoScene *scene, CChoreoEvent *event, CBaseEntity *pTarget)
	{
		DevMsg("[TICK %7d] CBaseFlex::AddSceneEvent [type: %s] [name: %s]\n", gpGlobals->tickcount,
			CChoreoEvent::NameForType(event->GetType()), event->GetName());
		DETOUR_MEMBER_CALL(scene, event, pTarget);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:UserMsg_Overflow_v2")
		{
		#if 0
			using trace_pair = std::pair<Library, const char *>;
			for (const auto& trace : {
				trace_pair(Library::SERVER, "CTFPlayer\\w+PlayTauntRemapInputScene"           ),
				trace_pair(Library::SERVER, "CSceneEntity\\w+CancelPlaybackEv"                ),
				trace_pair(Library::SERVER, "CSceneEntity\\w+OnSceneFinished"                 ),
				trace_pair(Library::SERVER, "CSceneEntity\\w+ClearSceneEvents"                ),
				trace_pair(Library::SERVER, "CBaseFlex\\w+ClearSceneEvents"                   ),
				trace_pair(Library::SERVER, "CBaseFlex\\w+ClearSceneEventE"                   ),
				trace_pair(Library::SERVER, "CBaseEntity\\w+StopSoundEiiPKc"                  ),
				trace_pair(Library::SERVER, "CSoundEmitterSystem\\w+StopSoundByHandle"        ),
				trace_pair(Library::ENGINE, "CEngineSoundServer\\w+StopSoundE"                ),
				trace_pair(Library::ENGINE, "CEngineSoundServer\\w+EmitSoundE\\w+soundlevel_t"),
				trace_pair(Library::ENGINE, "SV_StartSound"                                   ),
				trace_pair(Library::ENGINE, "CGameServer\\w+BroadcastSound"                   ),
				trace_pair(Library::ENGINE, "CGameClient\\w+SendSound"                        ),
				
				trace_pair(Library::SERVER, "CBaseFlex\\w+AddSceneEvent"                      ),
				trace_pair(Library::SERVER, "CBaseFlex\\w+RemoveSceneEvent"                   ),
			}) {
				this->AddDetour(new CFuncTrace(trace.first, trace.second));
			}
		#endif
			
			MOD_ADD_DETOUR_MEMBER(CNetChan_SendNetMsg, "CNetChan::SendNetMsg");
			
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_PlayTauntRemapInputScene, "CTFPlayer::PlayTauntRemapInputScene");
			
			MOD_ADD_DETOUR_MEMBER(CBaseFlex_AddSceneEvent, "CBaseFlex::AddSceneEvent");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_usermsg_overflow_v2", "0", FCVAR_NOTIFY,
		"Debug: buffer overflow in net message",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// detour CNetChan::SendNetMsg
// print info about the INetChannel itself (which client it's for, etc)
// output before-and-after stats for CNetChan::m_StreamReliable
// also output info about msg
// if num bits written exceeds a certain threshold, then BACKTRACE()





// taunt bug investigation:

// reproduce:
// - do mannrobics taunt, stand there for a while
// - then hit mouse1, or space

// shitton of svc_Sounds messages!

// CBaseFlex::ClearSceneEvent is getting hammered by one call to CBaseFlex::ClearSceneEvents

// when congaing, for the first ~10 seconds, it's all fine
// but then, we start getting a call to CTFPlayer::PlayTauntRemapInputScene every single frame!

/*

[  21681] SendNetMsg[192.168.1.2:27005]: [b:       0] [a:      55] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:      55] [a:     110] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     110] [a:     165] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     165] [a:     220] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     220] [a:     275] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     275] [a:     330] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     330] [a:     385] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     385] [a:     440] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     440] [a:     495] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     495] [a:     550] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     550] [a:     605] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     605] [a:     660] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     660] [a:     715] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     715] [a:     770] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     770] [a:     825] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     825] [a:     880] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     880] [a:     935] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     935] [a:     990] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:     990] [a:    1045] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:    1045] [a:    1100] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:    1100] [a:    1155] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:    1155] [a:    1210] [d:      55] svc_Sounds: number 1, reliable, bytes 5
...
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627088] [a:  627143] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627143] [a:  627198] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627198] [a:  627253] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627253] [a:  627308] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627308] [a:  627363] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627363] [a:  627418] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627418] [a:  627473] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627473] [a:  627528] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627528] [a:  627583] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627583] [a:  627638] [d:      55] svc_Sounds: number 1, reliable, bytes 5
[  21681] SendNetMsg[192.168.1.2:27005]: [b:  627638] [a:  627671] [d:      33] svc_UserMessage: type 71, bytes 1




FRM  ESP       EIP       FUNC
  1  ffde74a0  ddd938b4  Mod_Debug_UserMsg_Overflow_v2::Detour_CNetChan_SendNetMsg::CNetChan_SendNetMsg(INetMessage&, bool, bool)+0xea
  2  ffde74f0  f5d2d76d  CBaseClient::SendNetMsg(INetMessage&, bool)+0x7d
  3  ffde7530  f5e08f8d  CGameClient::SendSound(SoundInfo_t&, bool)+0x1dd
  4  ffde7650  f5e18f6e  CGameServer::BroadcastSound(SoundInfo_t&, IRecipientFilter&)+0xfe
  5  ffde76a0  f5e1c49e  SV_StartSound(IRecipientFilter&, edict_t*, int, char const*, float, soundlevel_t, int, int, int, Vector const*, float, int, CUtlVector<Vector, CUtlMemory<Vector, int> >*)+0x21e
  6  ffde7760  f5d783dc  CEngineSoundServer::EmitSound(IRecipientFilter&, int, int, char const*, float, soundlevel_t, int, int, int, Vector const*, Vector const*, CUtlVector<Vector, CUtlMemory<Vector, int> >*, bool, float, int)+0x2fc
  7  ffde77f0  f5d77c0f  CEngineSoundServer::StopSound(int, int, char const*)+0x9f
  8  ffde7860  f0ef59d0  CSoundEmitterSystem::StopSoundByHandle(int, char const*, short&) [clone .part.45]+0x80
  9  ffde78b0  f0ef61ea  CBaseEntity::StopSound(int, char const*)+0x3a
 10  ffde78e0  f11cc818  CBaseFlex::ClearSceneEvent(CSceneEventInfo*, bool, bool)+0x198
 11  ffde7930  f11d2f39  CBaseFlex::ClearSceneEvents(CChoreoScene*, bool)+0x89
 12  ffde7980  f13748e6  CSceneEntity::ClearSceneEvents(CChoreoScene*, bool)+0x86
 13  ffde79c0  f137742a  CSceneEntity::OnSceneFinished(bool, bool)+0x17a
 14  ffde7a00  f1371b6b  CSceneEntity::CancelPlayback()+0xeb
 15  ffde7a40  f156af47  CTFPlayer::PlayTauntRemapInputScene()+0x167
 16  ffde7ac0  f156b1ac  CTFPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)+0x15c
 17  ffde7b30  f133bc67  CBasePlayer::PhysicsSimulate()+0x607
 18  ffde7c00  f130bd0b  Physics_SimulateEntity(CBaseEntity*)+0x45b
 19  ffde7c90  f130c1ca  Physics_RunThinkFunctions(bool)+0x22a
 20  ffde7e00  f123f791  CServerGameDLL::GameFrame(bool)+0x161
 21  ffde7e80  eae6773a  __SourceHook_FHCls_IServerGameDLLGameFramefalse::Func(bool)+0x17a
 22  ffde7ed0  f5e1acb7  SV_Think(bool)+0x1b7
 23  ffde7f50  f5e1beb9  SV_Frame(bool)+0x179
 24  ffde7fc0  f5d8e5a7  _Host_RunFrame_Server(bool)+0x177
 25  ffde8050  f5d8f2b9  _Host_RunFrame(float)+0x449
 26  ffde8150  f5d9cf31  CHostState::State_Run(float)+0xe1
 27  ffde8170  f5d9d17e  CHostState::FrameUpdate(float)+0x9e
 28  ffde81a0  f5d9d289  HostState_Frame(float)+0x19
 29  ffde81c0  f5e3326e  CEngine::Frame()+0x38e
 30  ffde8290  f5e30286  CDedicatedServerAPI::RunFrame()+0x26
 31  ffde82b0  f6c0f842  RunServer()+0x42
 32  ffde82d0  f5e3037d  CModAppSystemGroup::Main()+0x9d
 33  ffde82f0  f5e71e40  CAppSystemGroup::Run()+0x30
 34  ffde8320  f5e3107f  CDedicatedServerAPI::ModInit(ModInfo_t&)+0x1df
 35  ffde8350  f6c0f5a3  CDedicatedAppSystemGroup::Main()+0x93
 36  ffde8390  f6c303b0  CAppSystemGroup::Run()+0x30
 37  ffde83c0  f6c303b0  CAppSystemGroup::Run()+0x30
 38  ffde83f0  f6bd4c06  main+0x1c6
 39  ffde86f0  080489b9  main+0x239
 40  ffdeb750  f733f536  __libc_start_main+0xf6
 41  ffdeb7c0  08048b51  _start+0x21
*/
