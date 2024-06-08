#ifdef SE_IS_CSS
#include "mod.h"
#include "stub/baseplayer.h"
#include "util/misc.h"

    
namespace Mod::Perf::CSS_Bot_Vision_Optimize
{   
    // class CSPlayerBoneOffsets : public EntityModule 
    // {
    // public:
    //     CSPlayerBoneOffsets(CBaseEntity *owner) : EntityModule(owner) {}
    //     Vector boneOffsets[5];
    //     QAngle boneAngleStored[5];
    //     Vector bonePosCalculated[5];
    //     int boneOffsetsTick[5] {0,0,0,0,0};
    // };

    enum VisiblePartType
    {
        NONE		= 0x00,
        GUT			= 0x01,
        HEAD		= 0x02,
        LEFT_SIDE	= 0x04,
        RIGHT_SIDE	= 0x08,
        FEET		= 0x10
    };
    
    RefCount rc_CCSBot_IsVisible;
    Vector gut_pos;
    Vector head_pos;
    Vector left_side_pos;
    Vector right_side_pos;
    Vector feet_pos;

    static byte m_PVS[PAD_NUMBER( MAX_MAP_CLUSTERS,8 ) / 8];
    static int m_nPVSSize;

    DETOUR_DECL_MEMBER(Vector *, CCSBot_GetPartPosition, CBasePlayer *player, int partType)
    {
        if (!rc_CCSBot_IsVisible) return DETOUR_MEMBER_CALL(player,partType);
        
        switch (partType) {
            case GUT: return &gut_pos;
            case HEAD: return &head_pos;
            case LEFT_SIDE: return &left_side_pos;
            case RIGHT_SIDE: return &right_side_pos;
            case FEET: return &feet_pos;
            default: return &feet_pos;
        }
        // auto mod = me->GetOrCreateEntityModule<CSPlayerBoneOffsets>("boneoffsets");
        // int partTypeIndex = 0;
        // switch (partType) {
        //     case 1: partTypeIndex = 0; break;
        //     case 2: partTypeIndex = 1; break;
        //     case 4: partTypeIndex = 2; break;
        //     case 8: partTypeIndex = 3; break;
        //     case 16: partTypeIndex = 4; break;
        // }
        // if (gpGlobals->tickcount > mod->boneOffsetsTick[partTypeIndex] + 6) {
        //     auto vec = DETOUR_MEMBER_CALL(player,partType);
        //     mod->boneOffsets[partTypeIndex] = *vec - player->GetAbsOrigin(); 
        //     //mod->boneAngleStored[partTypeIndex] = player->GetAbsAngles(); 
        //     mod->boneOffsetsTick[partTypeIndex] = gpGlobals->tickcount;
        //     return vec;
        // }
        // else {
        //     auto boneOffset = mod->boneOffsets[partTypeIndex];
        //     auto bonePosCalculated = &mod->bonePosCalculated[partTypeIndex];
        //     //Vector boneOffsetResult;
        //     //VectorRotate(boneOffset, player->GetAbsAngles() - mod->boneAngleStored[partTypeIndex], boneOffsetResult);
        //     *bonePosCalculated = player->GetAbsOrigin() + boneOffset;
        //     //auto vec = DETOUR_MEMBER_CALL(player,partType);
        //     //Msg("Diff %f %f %f\n", vec->x - bonePosCalculated->x, vec->y - bonePosCalculated->y, vec->z - bonePosCalculated->z);
        //     return bonePosCalculated;
        // }
        //auto vec = DETOUR_MEMBER_CALL(player,partType);
        //Msg("Part %d offset %f %f %f\n", partType, vec->x - player->GetAbsOrigin().x, vec->y - player->GetAbsOrigin().y, vec->z - player->GetAbsOrigin().z);
        //return vec;
    }

    DETOUR_DECL_MEMBER(CBasePlayer *, CSBot_FindMostDangerousThreat)
    {
        m_nPVSSize = sizeof( m_PVS );
        engine->ResetPVS(m_PVS, m_nPVSSize);
        auto me = reinterpret_cast<CBasePlayer *>(this);
        engine->AddOriginToPVS(me->EyePosition());
        return DETOUR_MEMBER_CALL();
    }

    DETOUR_DECL_MEMBER(bool, CCSBot_IsVisible, CBasePlayer *player, bool testFov, unsigned char *visParts)
    {
        SCOPED_INCREMENT(rc_CCSBot_IsVisible);
        auto me = reinterpret_cast<CBasePlayer *>(this);
        auto absPos = player->GetAbsOrigin();
        if (!engine->CheckBoxInPVS( player->CollisionProp()->OBBMins()+absPos, player->CollisionProp()->OBBMaxs()+absPos, m_PVS, m_nPVSSize)) return false;

        auto eyePos = player->EyePosition();
        auto OBBMaxZ = player->CollisionProp()->OBBMaxs().z;

        Vector fwd;
        Vector right;
        AngleVectors(player->GetAbsAngles(), &fwd, &right, nullptr);

        feet_pos = absPos + Vector(0,0,5);
        head_pos = eyePos + fwd * 9 - Vector(0,0,2);
        gut_pos = absPos + Vector(0,0,OBBMaxZ * 0.7f) - fwd * 3;
        left_side_pos = absPos + Vector(0,0,OBBMaxZ * 0.73f) + fwd * 14 - right * 9;
        right_side_pos = absPos + Vector(0,0,OBBMaxZ * 0.73f) + fwd * 10 + right * 13;

        // {
        //     TIME_SCOPE2(potentiallyvisiblecalc);
        //     auto me = reinterpret_cast<CBasePlayer *>(this);
        //     Vector origin = player->GetAbsOrigin();
        //     Vector center = player->CollisionProp()->OBBCenter();
        //     if (testFov && !me->FInViewCone(origin+center)) return false;
        //     Vector ourEyePos = me->EyePosition();
        //     Vector mins = player->CollisionProp()->OBBMins();
        //     Vector maxs = player->CollisionProp()->OBBMaxs();
        //     Vector fwd;
        //     Vector right;
        //     AngleVectors(player->GetAbsAngles(), &fwd, &right, nullptr);

        //     bool potentiallyVisible = false;
        //     for (const Vector &pos : {origin + Vector(0,0,5), maxs + origin, Vector(maxs.x, mins.y, maxs.z) + origin, Vector(mins.x, mins.y, maxs.z) + origin, Vector(mins.x, maxs.y, mins.z + 8) + origin, center + origin, player->EyePosition(), origin + Vector(0,0,45) + fwd * 60, origin + Vector(0,0,45) + right * 60, origin + Vector(0,0,45) - right * 60} ) {
        //         trace_t result;
        //         CTraceFilterIgnorePlayers traceFilter( nullptr, COLLISION_GROUP_NONE );
        //         UTIL_TraceLine( ourEyePos, pos, MASK_VISIBLE_AND_NPCS, &traceFilter, &result );
        //         //Msg("Our %f %f %f\n", pos.x - player->GetAbsOrigin().x, pos.y - player->GetAbsOrigin().y, pos.z - player->GetAbsOrigin().z);
        //         if (result.fraction == 1.0f) {
        //             potentiallyVisible = true;
        //             break;
        //         }
        //     }
        //     //if (!potentiallyVisible) return false;
        // }
        // TIME_SCOPE2(realcalc);
        // trace_t result;
        // CTraceFilterNoNPCsOrPlayer traceFilter( ignore, COLLISION_GROUP_NONE );
        // UTIL_TraceLine( EyePositionConst(), pos, MASK_VISIBLE_AND_NPCS, &traceFilter, &result );

        // if (result.fraction != 1.0f)
        // return false;

        // if (myArea != nullptr && theirArea != nullptr) {
        //     bool isVisible = myArea->IsPotentiallyVisible(theirArea);
        //     Msg("\n");
        //     if (!isVisible) {
        //         return false;
        //     }
        // }
        auto ret = DETOUR_MEMBER_CALL(player,testFov,visParts);
        // static int last_tick = 0;
        // static int visible_potentially = 0;
        // static int visible_potentially_not = 0;
        // if (ret) {
        //     if (potentiallyVisible) {
        //         visible_potentially++;
        //     }
        //     else {
        //         visible_potentially_not++;
        //     }
        // }
        // if (last_tick != gpGlobals->tickcount) {
        //     Msg("Potentially visible test: correct %d fail %d\n", visible_potentially, visible_potentially_not);
        // }
        // last_tick = gpGlobals->tickcount;
        return ret;
    }
    class CMod : public IMod
    {
    public:
        CMod() : IMod("Perf::CSS_Bot_Vision_Optimize")
        {
            MOD_ADD_DETOUR_MEMBER(CCSBot_IsVisible, "CCSBot::IsVisible");
            MOD_ADD_DETOUR_MEMBER(CCSBot_GetPartPosition, "CCSBot::GetPartPosition");
            MOD_ADD_DETOUR_MEMBER(CSBot_FindMostDangerousThreat, "CCSBot::FindMostDangerousThreat");
        }
    };
    CMod s_Mod;
    
    
    ConVar cvar_enable("sig_perf_css_bot_vision_optimize", "0", FCVAR_NOTIFY,
        "Mod: Optimize CS:S bot vision performance, but makes it also slighty less accurate",
        [](IConVar *pConVar, const char *pOldValue, float flOldValue){
            s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
        });
}
#endif