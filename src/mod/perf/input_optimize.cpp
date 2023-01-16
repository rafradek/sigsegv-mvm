#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/tfweaponbase.h"
#include "stub/tf_objective_resource.h"
#include "sdk2013/mempool.h"
#include "mem/protect.h"
#include "mod/common/commands.h"

#define EVENT_FIRE_ALWAYS	-1

namespace Mod::Perf::Input_Optimize
{

    ConVar cvar_debug_enable("sig_cvar_perf_input_optimize_print_debug", "0", FCVAR_NOTIFY,
		"Print debug messages to admins");
    
    DETOUR_DECL_MEMBER(void, CEventQueue_ServiceEvents)
    {
        CFastTimer timer;
        timer.Start();
        DETOUR_MEMBER_CALL(CEventQueue_ServiceEvents)();
        timer.End();
        DevMsg("serviceevents %.9f\n", timer.GetDuration().GetSeconds());
    }

    GlobalThunk<CUtlMemoryPool> g_EntityListPool("g_EntityListPool");

    std::set<CHandle<CBasePlayer>> message_listeners;
    bool message_listeners_empty = true;

    template<typename... ARGS>
    void SendMessageToListeners(const char *fmt, ARGS&&... args)
    {
        CFmtStrN<1024> str(fmt, std::forward<ARGS>(args)...);
        for (auto &player : message_listeners) {
            if (player.ToInt() == INVALID_EHANDLE_INDEX) {
                Msg("%s\n", str);
            }
            else if (player != nullptr) {
                if (player->IsFakeClient())  continue;
                
                engine->ClientPrintf(player->edict(), str);
            }
        }
    }

    DETOUR_DECL_MEMBER(void, CBaseEntityOutput_FireOutput, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
    {
            //DETOUR_MEMBER_CALL(CBaseEntityOutput_FireOutput)(Value, pActivator, pCaller, fDelay);
        CBaseEntityOutput *output = reinterpret_cast<CBaseEntityOutput *>(this);
        //
        // Iterate through all eventactions and fire them off.
        //
        CEventAction *ev = output->m_ActionList;
        CEventAction *prev = NULL;
        
        while (ev != NULL)
        {
            if (ev->m_iParameter == NULL_STRING)
            {
                //
                // Post the event with the default parameter.
                //
                g_EventQueue.GetRef().AddEvent( STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), Value, ev->m_flDelay + fDelay, pActivator, pCaller, ev->m_iIDStamp );
            }
            else
            {
                //
                // Post the event with a parameter override.
                //
                variant_t ValueOverride;
                ValueOverride.SetString( ev->m_iParameter );
                g_EventQueue.GetRef().AddEvent( STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), ValueOverride, ev->m_flDelay, pActivator, pCaller, ev->m_iIDStamp );
            }

            if (!message_listeners_empty) {
                SendMessageToListeners(
                    "(%0.2f) output: (%s,%s) -> (%s,%s,%.1f)(%s)\n",
                    engine->GetServerTime(),
                    pCaller ? pCaller->GetClassname() : "NULL",
                    pCaller ? STRING(pCaller->GetEntityName()) : "NULL",
                    STRING(ev->m_iTarget),
                    STRING(ev->m_iTargetInput),
                    ev->m_flDelay,
                    STRING(ev->m_iParameter) );
            }

            //
            // Remove the event action from the list if it was set to be fired a finite
            // number of times (and has been).
            //
            bool bRemove = false;
            if (ev->m_nTimesToFire != EVENT_FIRE_ALWAYS)
            {
                ev->m_nTimesToFire--;
                if (ev->m_nTimesToFire == 0)
                {
                    if (!message_listeners_empty) {
                        SendMessageToListeners("Removing from action list: (%s,%s) -> (%s,%s)\n", pCaller ? pCaller->GetClassname() : "NULL", pCaller ? STRING(pCaller->GetEntityName()) : "NULL", STRING(ev->m_iTarget), STRING(ev->m_iTargetInput));
                    }
                    bRemove = true;
                }
            }

            if (!bRemove)
            {
                prev = ev;
                ev = ev->m_pNext;
            }
            else
            {
                if (prev != NULL)
                {
                    prev->m_pNext = ev->m_pNext;
                }
                else
                {
                    output->m_ActionList = ev->m_pNext;
                }

                CEventAction *next = ev->m_pNext;
                g_EntityListPool.GetRef().Free(ev);
                //delete ev;
                ev = next;
            }
        }
    
        //DETOUR_MEMBER_CALL(CBaseEntityOutput_FireOutput)(Value, pActivator, pCaller, fDelay);
    }

    DETOUR_DECL_MEMBER(bool, CBaseEntity_AcceptInput, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
    {
        // loop through the data description list, restoring each data desc block
        //if (!cvar_real_enable.GetBool()) {
            //auto ret = DETOUR_MEMBER_CALL(CBaseEntity_AcceptInput)(szInputName, pActivator, pCaller, Value, outputID);
            //DevMsg("timeacceptinput %.9f\n", timer.GetDuration().GetSeconds());
            //return ret;
        //}
        CBaseEntity *ent = reinterpret_cast<CBaseEntity *>(this);
        for ( datamap_t *dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
        {
            // search through all the actions in the data description, looking for a match
            for ( int i = 0; i < dmap->dataNumFields; i++ )
            {
                if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
                {
                    if ( !Q_stricmp(dmap->dataDesc[i].externalName, szInputName) )
                    {
                        if (!message_listeners_empty) {
                            SendMessageToListeners("(%0.2f) input %s: %s.%s(%s)\n", gpGlobals->curtime, pCaller != nullptr ? STRING(pCaller->GetEntityName()) : "<no caller>", ent->GetEntityName(), szInputName, Value.String());
                        }

                        // convert the value if necessary
                        if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
                        {
                            if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
                            {
                                if ( !Value.Convert( (fieldtype_t)dmap->dataDesc[i].fieldType ) )
                                {
                                    if (!message_listeners_empty) {
                                        SendMessageToListeners("!! ERROR: bad input/output link:\n!! %s(%s,%s) doesn't match type from %s(%s)\n", 
                                            ent->GetClassname(), STRING(ent->GetEntityName()), szInputName, 
                                            ( pCaller != NULL ) ? pCaller->GetClassname() : "<null>",
                                            ( pCaller != NULL ) ? STRING(pCaller->GetEntityName()) : "<null>");
                                    }
                                    //timer.End();
                                    //DevMsg("timeacceptinput %.9f\n", timer.GetDuration().GetSeconds());
                                    return false;
                                }
                            }
                        }

                        // call the input handler, or if there is none just set the value
                        inputfunc_t pfnInput = dmap->dataDesc[i].inputFunc;

                        if ( pfnInput )
                        { 
                            // Package the data into a struct for passing to the input handler.
                            inputdata_t data;
                            data.pActivator = pActivator;
                            data.pCaller = pCaller;
                            data.value = Value;
                            data.nOutputID = outputID;

                            (ent->*pfnInput)( data );
                        }
                        else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
                        {
                            // set the value directly
                            Value.SetOther( ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
                            ent->NetworkStateChanged();
                        }
                        //timer.End();
                        //DevMsg("timeacceptinput %.9f\n", timer.GetDuration().GetSeconds());
                        return true;
                    }
                }
            }
        }
        if (!message_listeners_empty) {
            SendMessageToListeners("unhandled input: (%s) -> (%s,%s)\n", szInputName, ent->GetClassname(), STRING(ent->GetEntityName())/*,", from (%s,%s)" STRING(pCaller->m_iClassname), STRING(pCaller->m_iName)*/ );
        }
        return false;
    }

    class CMod : public IMod, IModCallbackListener
	{
	public:
		CMod() : IMod("Perf::Input_Optimize")
		{
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntity_AcceptInput, "CBaseEntity::AcceptInput", LOWEST);
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntityOutput_FireOutput, "CBaseEntityOutput::FireOutput", LOWEST);
            //MOD_ADD_DETOUR_MEMBER(CEventQueue_ServiceEvents, "CEventQueue::ServiceEvents");
            
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
        {
            message_listeners.clear();
            message_listeners_empty = true;
        }
	};
	CMod s_Mod;

	ModCommand sig_print_input("sig_print_input", [](CTFPlayer *player, const CCommand& args){
		int activate;
        if (args.ArgC() == 2 && StringToIntStrict(args[1], activate) && activate) {
            message_listeners.insert(player);
            message_listeners_empty = false;
            ModCommandResponse("Reading input/output debug info\n");
        }
        else {
            message_listeners.erase(player);
            message_listeners_empty = message_listeners.empty();
        }
	}, &s_Mod);
	
	ConVar cvar_enable("sig_perf_input_optimize", "0", FCVAR_NOTIFY,
		"Mod: Optimize input/output entity links",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}