#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfentities.h"
#include "stub/gamerules.h"
#include "stub/populators.h"
#include "stub/tfbot.h"
#include "stub/nextbot_cc.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "stub/strings.h"
#include "stub/server.h"
#include "stub/objects.h"
#include "stub/extraentitydata.h"
#include "stub/tf_objective_resource.h"
#include "mod/pop/common.h"
#include "util/pooled_string.h"
#include "util/scope.h"
#include "util/iterate.h"
#include "util/misc.h"
#include "util/clientmsg.h"
//#include <boost/algorithm/string.hpp>
//#include <boost/tokenizer.hpp>
#include <regex>
#include <string_view>
#include <optional>
#include <charconv>
#include "stub/sendprop.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/vi.h"
#include "util/expression_eval.h"
#include "mod/etc/mapentity_additions.h"

namespace Mod::Etc::Mapentity_Additions
{
    
    class CaseMenuHandler : public IMenuHandler
    {
    public:

        CaseMenuHandler(CTFPlayer * pPlayer, CLogicCase *pProvider) : IMenuHandler() {
            this->player = pPlayer;
            this->provider = pProvider;
        }

        void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item) {

            if (provider == nullptr)
                return;
                
            const char *info = menu->GetItemInfo(item, nullptr);

            provider->FireCase(item + 1, player);
            variant_t variant;
            provider->FireCustomOutput<"onselect">(player, provider, variant);
        }

        virtual void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
		{
            if (provider == nullptr)
                return;

            variant_t variant;
            provider->m_OnDefault->FireOutput(variant, player, provider);
		}

        virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
            HandleSecurity sec(g_Ext.GetIdentity(), g_Ext.GetIdentity());
			handlesys->FreeHandle(menu->GetHandle(), &sec);
		}

        void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }

        CHandle<CTFPlayer> player;
        CHandle<CLogicCase> provider;
    };

    void FireFormatInput(CLogicCase *entity, CBaseEntity *activator, CBaseEntity *caller)
    {
        std::string fmtstr = STRING(entity->m_nCase[15]);
        unsigned int pos = 0;
        unsigned int index = 1;
        while ((pos = fmtstr.find('%', pos)) != std::string::npos ) {
            if (pos != fmtstr.size() - 1 && fmtstr[pos + 1] == '%') {
                fmtstr.erase(pos, 1);
                pos++;
            }
            else
            {
                string_t str = entity->m_nCase[index - 1];
                fmtstr.replace(pos, 1, STRING(str));
                index++;
                pos += strlen(STRING(str));
            }
        }

        variant_t variant1;
        variant1.SetString(AllocPooledString(fmtstr.c_str()));
        entity->m_OnDefault->FireOutput(variant1, activator, entity);
    }

    THINK_FUNC_DECL(RotatingFollowEntity)
    {
        auto rotating = reinterpret_cast<CFuncRotating *>(this);
        auto data = GetExtraFuncRotatingData(rotating);
        if (data->m_hRotateTarget == nullptr)
            return;

        auto lookat = rotating->GetCustomVariable<"lookat">();
        Vector targetVec;
        if (FStrEq(lookat, PStr<"origin">())) {
            targetVec = data->m_hRotateTarget->GetAbsOrigin();
        } 
        else if (FStrEq(lookat, PStr<"center">())) {
            targetVec = data->m_hRotateTarget->WorldSpaceCenter();
        } 
        else {
            targetVec = data->m_hRotateTarget->EyePosition();
        }

        targetVec -= rotating->GetAbsOrigin();
        targetVec += data->m_hRotateTarget->GetAbsVelocity() * gpGlobals->frametime;
        float projectileSpeed = rotating->GetCustomVariableFloat<"projectilespeed">();
        if (projectileSpeed != 0) {
            targetVec += (targetVec.Length() / projectileSpeed) * data->m_hRotateTarget->GetAbsVelocity();
        }

        QAngle angToTarget;
	    VectorAngles(targetVec, angToTarget);

        float aimOffset = rotating->GetCustomVariableFloat<"aimoffset">();
        if (aimOffset != 0) {
            angToTarget.x -= Vector(targetVec.x, targetVec.y, 0.0f).Length() * aimOffset;
        }

        float angleDiff;
        float angle = 0;
        QAngle moveAngle = rotating->m_vecMoveAng;
        QAngle angles = rotating->GetAbsAngles();

        float limit = rotating->GetCustomVariableFloat<"limit">();
        if (limit != 0.0f) {
            angToTarget.x = clamp(AngleNormalize(angToTarget.x), -limit, limit);
            angToTarget.y = clamp(AngleNormalize(angToTarget.y), -limit, limit);
        }
        if (moveAngle == QAngle(1,0,0)) {
            angleDiff = AngleDiff(angles.x, angToTarget.x);
            angle = rotating->GetLocalAngles().x;
        }
        else {
            angleDiff = AngleDiff(angles.y, angToTarget.y);
            angle = rotating->GetLocalAngles().y;
        }

        float speed = rotating->m_flMaxSpeed;
        if (abs(angleDiff) < rotating->m_flMaxSpeed/66) {
            speed = 0;
            if (moveAngle == QAngle(1,0,0)) {
                rotating->SetAbsAngles(QAngle(angToTarget.x, angles.y, angles.z));
            }
            else {
                rotating->SetAbsAngles(QAngle(angles.x, angToTarget.y, angles.z));
            }
        }

        if (angleDiff > 0) {
            speed *= -1.0f;
        }

        if (speed != rotating->m_flTargetSpeed) {
            rotating->m_bReversed = angleDiff > 0;
            rotating->SetTargetSpeed(abs(speed));
        }
        
        rotating->SetNextThink(gpGlobals->curtime + 0.01f, "RotatingFollowEntity");
    }

    THINK_FUNC_DECL(RotatorModuleTick)
    {
        auto data = this->GetEntityModule<RotatorModule>("rotator");
        if (data == nullptr || data->m_hRotateTarget == nullptr)
            return;

        auto lookat = this->GetCustomVariable<"lookat">("eyes");
        Vector targetVec;
        auto aimEntity = data->m_hRotateTarget;
        if (FStrEq(lookat, PStr<"origin">())) {
            targetVec = data->m_hRotateTarget->GetAbsOrigin();
        } 
        else if (FStrEq(lookat, PStr<"center">())) {
            targetVec = data->m_hRotateTarget->WorldSpaceCenter();
        } 
        else if (FStrEq(lookat, PStr<"aim">())) {
            Vector fwd;
            Vector dest;
            AngleVectors(data->m_hRotateTarget->EyeAngles(), &fwd);
            VectorMA(data->m_hRotateTarget->EyePosition(), 8192.0f, fwd, dest);
            trace_t tr;
            UTIL_TraceLine(data->m_hRotateTarget->EyePosition(), dest, MASK_SHOT, data->m_hRotateTarget, COLLISION_GROUP_NONE, &tr);

            targetVec = tr.endpos;
            aimEntity = tr.m_pEnt;
        } 
        else {
            targetVec = data->m_hRotateTarget->EyePosition();
        }

        targetVec -= this->GetAbsOrigin();
        if (aimEntity != nullptr) {
            targetVec += aimEntity->GetAbsVelocity() * gpGlobals->frametime;
        }
        float projectileSpeed = this->GetCustomVariableFloat<"projectilespeed">();
        if (projectileSpeed != 0) {
            targetVec += (targetVec.Length() / projectileSpeed) * data->m_hRotateTarget->GetAbsVelocity();
        }

        QAngle angToTarget;
	    VectorAngles(targetVec, angToTarget);

        float aimOffset = this->GetCustomVariableFloat<"aimoffset">();
        if (aimOffset != 0) {
            angToTarget.x -= Vector(targetVec.x, targetVec.y, 0.0f).Length() * aimOffset;
        }

        QAngle angles = this->GetAbsAngles();

        bool velocitymode = this->GetCustomVariableFloat<"velocitymode">();
        float speedx = this->GetCustomVariableFloat<"rotationspeedx">();
        float speedy = this->GetCustomVariableFloat<"rotationspeedy">();
        if (!velocitymode) {
            speedx *= gpGlobals->frametime;
            speedy *= gpGlobals->frametime;
        }
        angToTarget.x = ApproachAngle(angToTarget.x, angles.x, speedx);
        angToTarget.y = ApproachAngle(angToTarget.y, angles.y, speedy);

        float limitx = this->GetCustomVariableFloat<"rotationlimitx">();
        if (limitx != 0.0f) {
            angToTarget.x = clamp(AngleNormalize(angToTarget.x), -limitx, limitx);
        }
        
        float limity = this->GetCustomVariableFloat<"rotationlimity">();
        if (limity != 0.0f) {
            angToTarget.y = clamp(AngleNormalize(angToTarget.y), -limity, limity);
        }

        if (!velocitymode) {
            this->SetAbsAngles(QAngle(angToTarget.x, angToTarget.y, angles.z));
        }
        else {
            angToTarget = QAngle(angToTarget.x - angles.x, angToTarget.y - angles.y, 0.0f);
            this->SetLocalAngularVelocity(angToTarget);
        }
        
        this->SetNextThink(gpGlobals->curtime + 0.01f, "RotatorModuleTick");
    }

    THINK_FUNC_DECL(ForwardVelocityTick)
    {
        if (this->GetEntityModule<ForwardVelocityModule>("forwardvelocity") == nullptr)
            return;
            
        Vector fwd;
        AngleVectors(this->GetAbsAngles(), &fwd);
        
        fwd *= this->GetCustomVariableFloat<"forwardspeed">();

        if (this->GetCustomVariableFloat<"directmode">() != 0) {
            this->SetAbsOrigin(this->GetAbsOrigin() + fwd * gpGlobals->frametime);
        }
        else {
            IPhysicsObject *pPhysicsObject = this->VPhysicsGetObject();
            if (pPhysicsObject) {
                pPhysicsObject->SetVelocity(&fwd, nullptr);
            }
            else {
                this->SetAbsVelocity(fwd);
            }
        }
        this->SetNextThink(gpGlobals->curtime + 0.01f, "ForwardVelocityTick");
    }

    ForwardVelocityModule::ForwardVelocityModule(CBaseEntity *entity)
    {
        if (entity->GetNextThink("ForwardVelocityTick") < gpGlobals->curtime) {
            THINK_FUNC_SET(entity, ForwardVelocityTick, gpGlobals->curtime + 0.01);
        }
    }

    THINK_FUNC_DECL(FakeParentModuleTick)
    {
        auto data = this->GetEntityModule<FakeParentModule>("fakeparent");
        if (data == nullptr) return;

        if (data->m_hParent == nullptr && data->m_bParentSet) {
            variant_t variant;
            this->FireCustomOutput<"onfakeparentkilled">(this, this, variant);
            data->m_bParentSet = false;
            if (data->m_bDeleteWithParent) {
                this->Remove();
                return;
            }
        }

        if (data->m_hParent == nullptr) return;

        Vector pos;
        QAngle ang;
        CBaseEntity *parent =data->m_hParent;

        auto bone = this->GetCustomVariable<"bone">();
        auto attachment = this->GetCustomVariable<"attachment">();
        bool posonly = this->GetCustomVariableFloat<"positiononly">();
        bool rotationonly = this->GetCustomVariableFloat<"rotationonly">();
        Vector offset = this->GetCustomVariableVector<"fakeparentoffset">();
        QAngle offsetangle = this->GetCustomVariableAngle<"fakeparentrotation">();
        matrix3x4_t transform;

        if (bone != nullptr) {
            CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(parent);
            anim->GetBoneTransform(anim->LookupBone(bone), transform);
        }
        else if (attachment != nullptr){
            CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(parent);
            anim->GetAttachment(anim->LookupAttachment(attachment), transform);
        }
        else{
            transform = parent->EntityToWorldTransform();
        }

        if (!rotationonly) {
            VectorTransform(offset, transform, pos);
            this->SetAbsOrigin(pos);
            this->SetAbsVelocity(vec3_origin);
        }

        if (!posonly) {
            MatrixAngles(transform, ang);
            ang += offsetangle;
            this->SetAbsAngles(ang);
        }

        this->SetNextThink(gpGlobals->curtime + 0.01f, "FakeParentModuleTick");
    }

    THINK_FUNC_DECL(AimFollowModuleTick)
    {
        auto data = this->GetEntityModule<AimFollowModule>("aimfollow");
        if (data == nullptr || data->m_hParent == nullptr) return;

        
        Vector fwd;
        Vector dest;
        AngleVectors(data->m_hParent->EyeAngles(), &fwd);
        VectorMA(data->m_hParent->EyePosition(), 8192.0f, fwd, dest);
        trace_t tr;
        CTraceFilterSkipTwoEntities filter(data->m_hParent, this, COLLISION_GROUP_NONE);
        UTIL_TraceLine(data->m_hParent->EyePosition(), dest, MASK_SHOT, &filter, &tr);

        Vector targetVec = tr.endpos;
        
        this->SetAbsOrigin(targetVec);
        bool rotationfollow = this->GetCustomVariableFloat<"rotationfollow">();
        if (rotationfollow) {
            this->SetAbsAngles(data->m_hParent->EyeAngles());
        }
        this->SetNextThink(gpGlobals->curtime + 0.01f, "AimFollowModuleTick");
    }

    THINK_FUNC_DECL(ScriptModuleTick)
    {
        auto data = this->GetEntityModule<ScriptModule>("script");
        if (data == nullptr) return;
        data->UpdateTimers();

        if (data->HasTimers()) {
            this->SetNextThink(gpGlobals->curtime + 0.01f, "ScriptModuleTick");
        }

    }

    inline CBaseEntity *FindTargetFromVariant(variant_t &Value, CBaseEntity *ent, CBaseEntity *pActivator, CBaseEntity *pCaller)
    {
        if (Value.FieldType() == FIELD_EHANDLE || Value.FieldType() == FIELD_CLASSPTR) {
            return Value.Entity().Get();
        }
        return servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
    }

    void ScriptModule::TimerAdded() {
        THINK_FUNC_SET(owner, ScriptModuleTick, gpGlobals->curtime);
    }

    void FireGetInput(CBaseEntity *entity, GetInputType type, const char *name, CBaseEntity *activator, CBaseEntity *caller, variant_t &value) {
        char param_tokenized[512] = "";
        V_strncpy(param_tokenized, value.String(), sizeof(param_tokenized));
        char *targetstr = strtok(param_tokenized,"|");
        char *action = strtok(NULL,"|");
        char *defvalue = strtok(NULL,"|");
        
        variant_t variable;

        if (targetstr != nullptr && action != nullptr) {

            bool found = GetEntityVariable(entity, type, name, variable);

            if (!found && defvalue != nullptr) {
                variable.SetString(AllocPooledString(defvalue));
            }

            if (found || defvalue != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, entity, activator, caller)) != nullptr ;) {
                    target->AcceptInput(action, activator, entity, variable, 0);
                }
            }
        }
    }

    ClassnameFilter logic_case_filter("logic_case", {
        {"FormatInputNoFire"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            int num = strtol(szInputName + strlen("FormatInputNoFire"), nullptr, 10);
            if (num > 0 && num < 16) {
                logic_case->m_nCase[num - 1] = AllocPooledString(Value.String());
            }
        }},
        {"FormatInput"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            int num = strtol(szInputName + strlen("FormatInput"), nullptr, 10);
            if (num > 0 && num < 16) {
                logic_case->m_nCase[num - 1] = AllocPooledString(Value.String());
                FireFormatInput(logic_case, pActivator, pCaller);
            }
        }},
        {"FormatString"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            logic_case->m_nCase[15] = AllocPooledString(Value.String());
            FireFormatInput(logic_case, pActivator, pCaller);
        }},
        {"FormatStringNoFire"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            logic_case->m_nCase[15] = AllocPooledString(Value.String());
        }},
        {"Format"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            FireFormatInput(logic_case, pActivator, pCaller);
        }},
        {"TestSigsegv"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->m_OnUser1->FireOutput(Value, pActivator, pCaller);
        }},
        {"ToInt"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            variant_t convert;
            convert.SetInt(strtol(Value.String(), nullptr, 10));
            logic_case->m_OnDefault->FireOutput(convert, pActivator, ent);
        }},
        {"ToFloat"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            variant_t convert;
            convert.SetFloat(strtof(Value.String(), nullptr));
            logic_case->m_OnDefault->FireOutput(convert, pActivator, ent);
        }},
        {"CallerToActivator"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            logic_case->m_OnDefault->FireOutput(Value, pCaller, ent);
        }},
        {"GetKeyValueFromActivator"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            variant_t variant;
            pActivator->ReadKeyField(Value.String(), &variant);
            logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
        }},
        {"GetConVar"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            ConVarRef cvref(Value.String());
            if (cvref.IsValid() && cvref.IsFlagSet(FCVAR_REPLICATED) && !cvref.IsFlagSet(FCVAR_PROTECTED)) {
                variant_t variant;
                variant.SetFloat(cvref.GetFloat());
                logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
            }
        }},
        {"GetConVarString"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            ConVarRef cvref(Value.String());
            if (cvref.IsValid() && cvref.IsFlagSet(FCVAR_REPLICATED) && !cvref.IsFlagSet(FCVAR_PROTECTED)) {
                variant_t variant;
                variant.SetString(AllocPooledString(cvref.GetString()));
                logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
            }
        }},
        {"DisplayMenu"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                if (target != nullptr && target->IsPlayer() && !ToTFPlayer(target)->IsBot()) {
                    CaseMenuHandler *handler = new CaseMenuHandler(ToTFPlayer(target), logic_case);
                    IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler, g_Ext.GetIdentity());

                    int i;
                    for (i = 1; i < 16; i++) {
                        string_t str = logic_case->m_nCase[i - 1];
                        const char *name = STRING(str);
                        if (strlen(name) != 0) {
                            bool enabled = name[0] != '!';
                            ItemDrawInfo info1(enabled ? name : name + 1, enabled ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
                            menu->AppendItem("it", info1);
                        }
                        else {
                            break;
                        }
                    }
                    if (i < 11) {
                        menu->SetPagination(MENU_NO_PAGINATION);
                    }

                    variant_t variant;
                    ent->ReadKeyField("Case16", &variant);
                    
                    char param_tokenized[256];
                    V_strncpy(param_tokenized, variant.String(), sizeof(param_tokenized));
                    
                    char *name = strtok(param_tokenized,"|");
                    char *timeout = strtok(NULL,"|");

                    menu->SetDefaultTitle(name);

                    char *flag;
                    while ((flag = strtok(NULL,"|")) != nullptr) {
                        if (FStrEq(flag, "Cancel")) {
                            menu->SetMenuOptionFlags(menu->GetMenuOptionFlags() | MENUFLAG_BUTTON_EXIT);
                        }
                    }

                    menu->Display(ENTINDEX(target), timeout == nullptr ? 0 : atoi(timeout));
                }
            }
        }},
        {"HideMenu"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto target = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (target != nullptr && target->IsPlayer()) {
                menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(target), false);
            }
        }},
        {"BitTest"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            for (int i = 1; i <= 16; i++) {
                string_t str = logic_case->m_nCase[i - 1];

                if (val & atoi(STRING(str))) {
                    logic_case->FireCase(i, pActivator);
                }
                else {
                    break;
                }
            }
        }},
        {"BitTestAll"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            for (int i = 1; i <= 16; i++) {
                string_t str = logic_case->m_nCase[i - 1];

                int test = atoi(STRING(str));
                if ((val & test) == test) {
                    logic_case->FireCase(i, pActivator);
                }
                else {
                    break;
                }
            }
        }}
    });
    ClassnameFilter tf_gamerules_filter("tf_gamerules", {
        {"StopVO"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            TFGameRules()->BroadcastSound(SOUND_FROM_LOCAL_PLAYER, Value.String(), SND_STOP);
        }},
        {"StopVORed"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            TFGameRules()->BroadcastSound(TF_TEAM_RED, Value.String(), SND_STOP);
        }},
        {"StopVOBlue"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            TFGameRules()->BroadcastSound(TF_TEAM_BLUE, Value.String(), SND_STOP);
        }},
        {"SetBossHealthPercentage"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_FLOAT);
            float val = Value.Float();
            if (g_pMonsterResource.GetRef() != nullptr)
                g_pMonsterResource->m_iBossHealthPercentageByte = (int) (val * 255.0f);
        }},
        {"SetBossState"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            if (g_pMonsterResource.GetRef() != nullptr)
                g_pMonsterResource->m_iBossState = val;
        }},
        {"AddCurrencyGlobal"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            TFGameRules()->DistributeCurrencyAmount(val, nullptr, true, true, false);
        }},
        {"ChangeLevel"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            char param_tokenized[256];
            V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
            
            char *map = strtok(param_tokenized,"|");
#ifndef NO_MVM
            char *mission = strtok(NULL,"|");
            change_level_info.set = mission != nullptr;
            change_level_info.mission = mission != nullptr ? mission : "";
            change_level_info.playerUpgradeHistory.clear();
            if (mission != nullptr) {
                change_level_info.startingCurrency = g_pPopulationManager->m_nStartingCurrency;
                change_level_info.currencyCollected = MannVsMachineStats_GetAcquiredCredits();
                ForEachTFPlayer([](CTFPlayer *player){
                    if (player->IsRealPlayer()) {
                        auto &history = *g_pPopulationManager->FindOrAddPlayerUpgradeHistory(player->GetSteamID());
                        auto &historySave = change_level_info.playerUpgradeHistory.emplace_back();
                        historySave.m_currencySpent = history.m_currencySpent;
                        historySave.m_steamId = history.m_steamId;
                        historySave.m_upgradeVector.insert(historySave.m_upgradeVector.end(), history.m_upgradeVector.begin(), history.m_upgradeVector.end());
                    }
                });
                Mod::Pop::PopMgr_Extensions::SaveStateInfoBetweenMissions();
            }
#endif
            engine->ChangeLevel(map, nullptr);
            //TFGameRules()->DistributeCurrencyAmount(val, nullptr, true, true, false);
        }},
        {"ReloadNavAttributes"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            TheNavMesh->RecomputeInternalData();
        }},
        {"CollectCash"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            // Distribute cash from packs
            int packsCurrency = TFObjectiveResource()->m_nMvMWorldMoney;
            
            for (int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i) {
                auto pack = rtti_cast<CCurrencyPack *>(ICurrencyPackAutoList::AutoList()[i]);
                if (pack == nullptr) continue;
                pack->SetDistributed(true);
                pack->Remove();
            }
            TFObjectiveResource()->m_nMvMWorldMoney = 0;
            TFGameRules()->DistributeCurrencyAmount( packsCurrency, NULL, true, false, false );
        }},
        {"FinishWave"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            if (g_pPopulationManager != nullptr) {
                CWave *wave = g_pPopulationManager->GetCurrentWave();
                ForEachTFBot([&](CTFBot *bot){
					if (bot->GetTeamNumber() != TEAM_SPECTATOR) {
						bot->CommitSuicide();
						bot->ForceChangeTeam(TEAM_SPECTATOR, true);
					}
				});
                if (wave != nullptr) {
                    int unallocatedCurrency = 0;
					FOR_EACH_VEC(wave->m_WaveSpawns, i) {
						CWaveSpawnPopulator *wavespawn = wave->m_WaveSpawns[i];
						if (wavespawn->m_unallocatedCurrency != -1) {
							unallocatedCurrency += wavespawn->m_unallocatedCurrency;
						}
					}
					TFGameRules()->DistributeCurrencyAmount( unallocatedCurrency, NULL, true, true, false );

				    wave->ForceFinish();
				    wave->ForceFinish();
				    wave->WaveCompleteUpdate();
                }
            }
        }},
        {"FinishWaveNoUnspawnedMoney"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            if (g_pPopulationManager != nullptr) {
                CWave *wave = g_pPopulationManager->GetCurrentWave();
                ForEachTFBot([&](CTFBot *bot){
					if (bot->GetTeamNumber() != TEAM_SPECTATOR) {
						bot->CommitSuicide();
						bot->ForceChangeTeam(TEAM_SPECTATOR, true);
					}
				});
                if (wave != nullptr) {
				    wave->ForceFinish();
				    wave->ForceFinish();
				    wave->WaveCompleteUpdate();
                }
            }
        }},
        {"JumpToWave"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            if (g_pPopulationManager != nullptr) {
                waveToJumpNextTick = val - 1;
                waveToJumpNextTickMoney = val > 1 ? -1.0f : 1.0f;
            }
        }},
        {"JumpToWaveCalculateMoney"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int val = Value.Int();
            if (g_pPopulationManager != nullptr) {
                waveToJumpNextTick = val - 1;
                waveToJumpNextTickMoney = 1.0f;
            }
        }}
    });
    ClassnameFilter player_filter("player", {
        {"AllowClassAnimations"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr) {
                player->GetPlayerClass()->m_bUseClassAnimations = Value.Bool();
            }
        }},
        {"SwitchClass"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            
            if (player != nullptr) {
                // Disable setup to allow class changing during waves in mvm
                static ConVarRef endless("tf_mvm_endless_force_on");
                endless.SetValue(true);

                int index = strtol(Value.String(), nullptr, 10);
                if (index > 0 && index < 10) {
                    player->HandleCommand_JoinClass(g_aRawPlayerClassNames[index]);
                }
                else {
                    player->HandleCommand_JoinClass(Value.String());
                }
                endless.SetValue(false);
            }
        }},
        {"SwitchClassInPlace"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            
            if (player != nullptr) {
                // Disable setup to allow class changing during waves in mvm
                static ConVarRef endless("tf_mvm_endless_force_on");
                endless.SetValue(true);

                Vector pos = player->GetAbsOrigin();
                QAngle ang = player->GetAbsAngles();
                Vector vel = player->GetAbsVelocity();

                int index = strtol(Value.String(), nullptr, 10);
                int oldState = player->m_Shared->GetState();
                player->m_Shared->SetState(TF_STATE_DYING);
                if (index > 0 && index < 10) {
                    player->HandleCommand_JoinClass(g_aRawPlayerClassNames[index]);
                }
                else {
                    player->HandleCommand_JoinClass(Value.String());
                }
                player->m_Shared->SetState(oldState);
                endless.SetValue(false);
                player->ForceRespawn();
                player->Teleport(&pos, &ang, &vel);
                
            }
        }},
        {"ForceRespawn"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr) {
                if (player->GetTeamNumber() >= TF_TEAM_RED && player->GetPlayerClass() != nullptr && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED) {
                    player->ForceRespawn();
                }
                else {
                    player->m_bAllowInstantSpawn = true;
                }
            }
        }},
        {"ForceRespawnDead"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr && !player->IsAlive()) {
                if (player->GetTeamNumber() >= TF_TEAM_RED && player->GetPlayerClass() != nullptr && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED) {
                    player->ForceRespawn();
                }
                else {
                    player->m_bAllowInstantSpawn = true;
                }
            }
        }},
        {"DisplayTextCenter"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            using namespace std::string_literals;
            std::string text{Value.String()};
            text = std::regex_replace(text, std::regex{"\\{newline\\}", std::regex_constants::icase}, "\n");
            text = std::regex_replace(text, std::regex{"\\{player\\}", std::regex_constants::icase}, ToTFPlayer(ent)->GetPlayerName());
            text = std::regex_replace(text, std::regex{"\\{activator\\}", std::regex_constants::icase}, 
                    (pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : ToTFPlayer(ent))->GetPlayerName());
            gamehelpers->TextMsg(ENTINDEX(ent), TEXTMSG_DEST_CENTER, text.c_str());
        }},
        {"DisplayTextChat"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            using namespace std::string_literals;
            std::string text{"\x01"s + Value.String() + "\x01"s};
            text = std::regex_replace(text, std::regex{"\\{reset\\}", std::regex_constants::icase}, "\x01");
            text = std::regex_replace(text, std::regex{"\\{blue\\}", std::regex_constants::icase}, "\x07" "99ccff");
            text = std::regex_replace(text, std::regex{"\\{red\\}", std::regex_constants::icase}, "\x07" "ff3f3f");
            text = std::regex_replace(text, std::regex{"\\{green\\}", std::regex_constants::icase}, "\x07" "99ff99");
            text = std::regex_replace(text, std::regex{"\\{darkgreen\\}", std::regex_constants::icase}, "\x07" "40ff40");
            text = std::regex_replace(text, std::regex{"\\{yellow\\}", std::regex_constants::icase}, "\x07" "ffb200");
            text = std::regex_replace(text, std::regex{"\\{grey\\}", std::regex_constants::icase}, "\x07" "cccccc");
            text = std::regex_replace(text, std::regex{"\\{newline\\}", std::regex_constants::icase}, "\n");
            text = std::regex_replace(text, std::regex{"\\{player\\}", std::regex_constants::icase}, ToTFPlayer(ent)->GetPlayerName());
            text = std::regex_replace(text, std::regex{"\\{activator\\}", std::regex_constants::icase}, 
                    (pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : ToTFPlayer(ent))->GetPlayerName());
            auto pos{text.find("{")};
            while(pos != std::string::npos){
                if(text.substr(pos).length() > 7){
                    text[pos] = '\x07';
                    text.erase(pos+7, 1);
                    pos = text.find("{");
                } else break; 
            }
            gamehelpers->TextMsg(ENTINDEX(ent), TEXTMSG_DEST_CHAT , text.c_str());
        }},
        {"DisplayTextHint"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            using namespace std::string_literals;
            std::string text{Value.String()};
            text = std::regex_replace(text, std::regex{"\\{newline\\}", std::regex_constants::icase}, "\n");
            text = std::regex_replace(text, std::regex{"\\{player\\}", std::regex_constants::icase}, ToTFPlayer(ent)->GetPlayerName());
            text = std::regex_replace(text, std::regex{"\\{activator\\}", std::regex_constants::icase}, 
                    (pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : ToTFPlayer(ent))->GetPlayerName());
            gamehelpers->HintTextMsg(ENTINDEX(ent), text.c_str());
        }},
        {"Suicide"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr) {
                player->CommitSuicide(false, true);
            }
        }},
        {"ChangeAttributes"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFBot *bot = ToTFBot(ent);
            if (bot != nullptr) {
                auto *attrib = bot->GetEventChangeAttributes(Value.String());
                if (attrib != nullptr){
                    bot->OnEventChangeAttributes(attrib);
                }
            }
        }},
        {"RollCommonSpell"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
            
            if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return;

            CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
            spellbook->RollNewSpell(0);
        }},
        {"SetSpell"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
            
            if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return;
            
            int index = Value.Int();
            if (Value.FieldType() == FIELD_STRING) {
                const char *str = Value.String();
                for (size_t i = 0; i < ARRAYSIZE(SPELL_TYPE); i++) {
                    if (FStrEq(str, SPELL_TYPE[i])) {
                        index = i;
                    }
                }
            }

            CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
            spellbook->m_iSelectedSpellIndex = index;
            spellbook->m_iSpellCharges = 1;
        }},
        {"AddSpell"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            
            CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
            
            if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return;
            
            int index = Value.Int();
            if (Value.FieldType() == FIELD_STRING) {
                const char *str = Value.String();
                for (size_t i = 0; i < ARRAYSIZE(SPELL_TYPE); i++) {
                    if (FStrEq(str, SPELL_TYPE[i])) {
                        index = i;
                    }
                }
            }

            CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
            if (spellbook->m_iSelectedSpellIndex != index) {
                spellbook->m_iSpellCharges = 0;
            }
            spellbook->m_iSelectedSpellIndex = index;
            spellbook->m_iSpellCharges += 1;
        }},
        {"AddCond"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            int index = 0;
            float duration = -1.0f;
            sscanf(Value.String(), "%d %f", &index, &duration);
            if (player != nullptr) {
                player->m_Shared->AddCond((ETFCond)index, duration);
            }
        }},
        {"RemoveCond"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            int index = strtol(Value.String(), nullptr, 10);
            if (player != nullptr) {
                player->m_Shared->RemoveCond((ETFCond)index);
            }
        }},
        {"AddPlayerAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            char param_tokenized[256];
            V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
            
            char *attr = strtok(param_tokenized,"|");
            char *value = strtok(NULL,"|");

            if (player != nullptr && value != nullptr) {
                player->AddCustomAttribute(attr, value, -1.0f);
            }
        }},
        {"RemovePlayerAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr) {
                player->RemoveCustomAttribute(Value.String());
            }
        }},
        {"GetPlayerAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
            if (player != nullptr) {
                char param_tokenized[512] = "";
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                char *attrName = strtok(param_tokenized,"|");
                char *targetstr = strtok(NULL,"|");
                char *action = strtok(NULL,"|");
                char *defvalue = strtok(NULL,"|");
                CEconItemAttribute * attr = player->GetAttributeList()->GetAttributeByName(attrName);
                variant_t variable;
                bool found = false;
                if (attr != nullptr) {
                    char buf[256];
                    attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
                    variable.SetString(AllocPooledString(buf));
                    found = true;
                }
                else {
                    variable.SetString(AllocPooledString(defvalue));
                }

                if (targetstr != nullptr && action != nullptr) {
                    if (found || defvalue != nullptr) {
                        for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                            target->AcceptInput(action, pActivator, ent, variable, 0);
                        }
                    }
                }
            }
        }},
        {"GetItemAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    char param_tokenized[512] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    char *itemSlot = strtok(param_tokenized,"|");
                    char *attrName = strtok(NULL,"|");
                    char *targetstr = strtok(NULL,"|");
                    char *action = strtok(NULL,"|");
                    char *defvalue = strtok(NULL,"|");

                    bool found = false;

                    variant_t variable;
                    if (itemSlot != nullptr && attrName != nullptr) {
                        int slot = 0;
                        CEconEntity *item = nullptr;
                        if (StringToIntStrict(itemSlot, slot)) {
                            if (slot != -1) {
                                item = GetEconEntityAtLoadoutSlot(player, slot);
                            }
                            else {
                                item = player->GetActiveTFWeapon();
                            }
                        }
                        else {
                            ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                                if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), itemSlot)) {
                                    item = entity;
                                }
                            });
                        }
                        
                        if (item != nullptr) {
                            CEconItemAttribute * attr = item->GetItem()->GetAttributeList().GetAttributeByName(attrName);
                            if (attr != nullptr) {
                                char buf[256];
                                attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
                                variable.SetString(AllocPooledString(buf));
                                found = true;
                            }
                        }
                    }

                    if (!found) {
                        variable.SetString(AllocPooledString(defvalue));
                    }

                    if (targetstr != nullptr && action != nullptr) {
                        if (found || defvalue != nullptr) {
                            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                                target->AcceptInput(action, pActivator, ent, variable, 0);
                            }
                        }
                    }
                }
        }},
        {"AddItemAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                char param_tokenized[512];
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                
                char *attr = strtok(param_tokenized,"|");
                char *value = strtok(NULL,"|");
                char *slot = strtok(NULL,"|");

                if (player != nullptr && value != nullptr) {
                    CEconEntity *item = nullptr;
                    if (slot != nullptr) {
                        ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                            if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), slot)) {
                                item = entity;
                            }
                        });
                        if (item == nullptr)
                            item = GetEconEntityAtLoadoutSlot(player, atoi(slot));
                    }
                    else {
                        item = player->GetActiveTFWeapon();
                    }
                    if (item != nullptr) {
                        CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(attr);
                        if (attr_def == nullptr) {
                            int idx = -1;
                            if (StringToIntStrict(attr, idx)) {
                                attr_def = GetItemSchema()->GetAttributeDefinition(idx);
                            }
                        }
                        if (attr_def != nullptr) {
                            item->GetItem()->GetAttributeList().AddStringAttribute(attr_def, value);
                        }
                    }
                }
        }},
        {"RemoveItemAttribute"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                char param_tokenized[256];
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                
                char *attr = strtok(param_tokenized,"|");
                char *slot = strtok(NULL,"|");

                if (player != nullptr) {
                    CEconEntity *item = nullptr;
                    if (slot != nullptr) {
                        ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                            if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), Value.String())) {
                                item = entity;
                            }
                        });
                        if (item == nullptr)
                            item = GetEconEntityAtLoadoutSlot(player, atoi(slot));
                    }
                    else {
                        item = player->GetActiveTFWeapon();
                    }
                    if (item != nullptr) {
                        CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(attr);
                        if (attr_def == nullptr) {
                            int idx = -1;
                            if (StringToIntStrict(attr, idx)) {
                                attr_def = GetItemSchema()->GetAttributeDefinition(idx);
                            }
                        }
                        item->GetItem()->GetAttributeList().RemoveAttribute(attr_def);

                    }
                }
        }},
        {"PlaySoundToSelf"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                CRecipientFilter filter;
                filter.AddRecipient(player);
                
                if (!enginesound->PrecacheSound(Value.String(), true))
                    CBaseEntity::PrecacheScriptSound(Value.String());

                EmitSound_t params;
                params.m_pSoundName = Value.String();
                params.m_flSoundTime = 0.0f;
                params.m_pflSoundDuration = nullptr;
                params.m_bWarnOnDirectWaveReference = true;
                CBaseEntity::EmitSound(filter, ENTINDEX(player), params);
        }},
        {"IgnitePlayerDuration"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_FLOAT);
                CTFPlayer *activator = pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : player;
                player->m_Shared->Burn(activator, nullptr, Value.Float());
        }},
        {"WeaponSwitchSlot"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                player->Weapon_Switch(player->Weapon_GetSlot(Value.Int()));
        }},
        {"WeaponStripSlot"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                int slot = Value.Int();
                CBaseCombatWeapon *weapon = player->GetActiveTFWeapon();
                if (slot != -1) {
                    weapon = player->Weapon_GetSlot(slot);
                }
                if (weapon != nullptr)
                    weapon->Remove();
        }},
        {"RemoveItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                    if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), Value.String())) {
                        if (entity->MyCombatWeaponPointer() != nullptr) {
                            player->Weapon_Detach(entity->MyCombatWeaponPointer());
                        }
                        entity->Remove();
                    }
                });
        }},
        {"GiveItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                GiveItemByName(player, Value.String());
        }},
        {"DropItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                int slot = Value.Int();
                CBaseCombatWeapon *weapon = player->GetActiveTFWeapon();
                if (slot != -1) {
                    weapon = player->Weapon_GetSlot(slot);
                }

                if (weapon != nullptr) {
                    CEconItemView *item_view = weapon->GetItem();

                    allow_create_dropped_weapon = true;
                    auto dropped = CTFDroppedWeapon::Create(player, player->EyePosition(), vec3_angle, weapon->GetWorldModel(), item_view);
                    if (dropped != nullptr)
                        dropped->InitDroppedWeapon(player, static_cast<CTFWeaponBase *>(weapon), false, false);

                    allow_create_dropped_weapon = false;
                }
        }},
        {"SetCurrency"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(player->GetCurrency() - atoi(Value.String()));
        }},
        {"AddCurrency"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(atoi(Value.String()) * -1);
        }},
        {"RemoveCurrency"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(atoi(Value.String()));
        }},
        {"CurrencyOutput"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                int cost = atoi(szInputName + 15);
                if(player->GetCurrency() >= cost){
                    char param_tokenized[2048] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    const char *separator = strchr(param_tokenized, ',') != nullptr ? "," : "|";
                    if(strcmp(param_tokenized, "") != 0){
                        char *target = strtok(param_tokenized,separator);
                        char *action = NULL;
                        char *value = NULL;
                        if(target != NULL)
                            action = strtok(NULL,separator);
                        if(action != NULL)
                            value = strtok(NULL,separator);
                        if(value != NULL){
                            CEventQueue &que = g_EventQueue;
                            variant_t actualvalue;
                            string_t stringvalue = AllocPooledString(value);
                            actualvalue.SetString(stringvalue);
                            que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, 0.0, player, player, -1);
                        }
                    }
                    player->RemoveCurrency(cost);
                }
        }},
        {"CurrencyInvertOutput"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer *player = ToTFPlayer(ent);
                int cost = atoi(szInputName + 21);
                if(player->GetCurrency() < cost){
                    char param_tokenized[2048] = "";
                    const char *separator = strchr(param_tokenized, ',') != nullptr ? "," : "|";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    if(strcmp(param_tokenized, "") != 0){
                        char *target = strtok(param_tokenized,separator);
                        char *action = NULL;
                        char *value = NULL;
                        if(target != NULL)
                            action = strtok(NULL,separator);
                        if(action != NULL)
                            value = strtok(NULL,separator);
                        if(value != NULL){
                            CEventQueue &que = g_EventQueue;
                            variant_t actualvalue;
                            string_t stringvalue = AllocPooledString(value);
                            actualvalue.SetString(stringvalue);
                            que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, 0.0, player, player, -1);
                        }
                    }
                    //player->RemoveCurrency(cost);
                }
        }},
        {"RefillAmmo"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                for(int i = 0; i < 7; ++i){
                    player->SetAmmoCount(player->GetMaxAmmo(i), i);
                }
        }},
        {"Regenerate"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                player->Regenerate(true);
        }},
        {"ResetInventory"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                
                player->GiveDefaultItemsNoAmmo();
        }},
        {"PlaySequence"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                player->PlaySpecificSequence(Value.String());
        }},
#ifndef NO_MVM
        {"AwardExtraItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                std::string str = Value.String();
                Mod::Pop::PopMgr_Extensions::AwardExtraItem(player, str, false);
        }},
        {"AwardAndGiveExtraItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                std::string str = Value.String();
                Mod::Pop::PopMgr_Extensions::AwardExtraItem(player, str, true);
        }},
        {"StripExtraItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                std::string str = Value.String();
                Mod::Pop::PopMgr_Extensions::StripExtraItem(player, str, true);
        }},
        {"ResetExtraItems"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                Mod::Pop::PopMgr_Extensions::ResetExtraItems(player);
        }},
#endif
        {"TauntFromItem2"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                
                CTFPlayer* player{ToTFPlayer(ent)};
                auto view{CEconItemView::Create()};
                const std::string_view input{Value.String()};
                auto index{vi::from_str<int>(Value.String())};
                const auto v{vi::split_str(input, "|")};
                const auto do_taunt{[&view, &player](int index) -> void {
                    view->Init(index);
                    player->PlayTauntSceneFromItem(view);
                    CEconItemView::Destroy(view);
                }};
                if(index && (v.size() < 2)){
                    do_taunt(*index);
                } else {
                    if(v.size() > 1){
                        index = {vi::from_str<int>(v[0])};
                        const auto value{v[1]};
                        if(index && (value.length() > 0)){
                            std::string_view no_op{value};
                            no_op.remove_prefix(1);
                            const auto remove_op{vi::from_str<float>(no_op)};
                            auto original{vi::from_str<float>(value)};
                            
                            const std::unordered_map<char,
                                        std::function<void()>> ops{
                                {'+', [&player, &remove_op]{
                                        player->m_flTauntAttackTime += *remove_op; 
                                    }
                                },
                                {'-', [&player, &remove_op]{
                                        player->m_flTauntAttackTime -= *remove_op; 
                                    }
                                },
                                {'*', [&player, &remove_op]{
                                        player->m_flTauntAttackTime =
                                            gpGlobals->curtime + 
                                            (player->m_flTauntAttackTime -
                                             gpGlobals->curtime) * 
                                            *remove_op; 
                                    }
                                }
                            };
                            if(value[0] == 'i'){
                                do_taunt(*index);
                                player->m_flTauntAttackTime = 0.1f;
                                original = {};
                            }
                            for(const auto& [op, func] : ops){
                                if((value[0] == op) && remove_op){
                                    do_taunt(*index);
                                    func();
                                    original = {};
                                    break;
                                }
                            }
                            if(original){
                                do_taunt(*index);
                                player->m_flTauntAttackTime =
                                    gpGlobals->curtime + *original;
                            }
                        }
                    }
                }
                
        }},
        {"TauntIndexConcept"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player{ToTFPlayer(ent)};
                const std::string_view input{Value.String()};
                auto v{vi::split_str(input, "|")};
                if(v.size() > 1){
                    auto index{
                        vi::from_str<int>(v[0])
                    };
                    auto taunt_concept{
                        vi::from_str<int>(v[1])
                    };
                    if(index && taunt_concept) 
                        player->Taunt((taunts_t)*index, *taunt_concept);
                }
                
        }},
        {"TauntFromItem"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player{ToTFPlayer(ent)};
                const std::string_view input{Value.String()};
                const auto v{vi::split_str(input, "|")};
                
                if (v.empty()) return;

                std::string itemName = std::string(v[0]);
		        auto item_def = GetItemSchema()->GetItemDefinitionByName(itemName.c_str());

                if (item_def == nullptr) return;

                CEconItemView *view = CEconItemView::Create();
                view->Init(item_def->m_iItemDefIndex, 6, 9999, 0);
#ifndef NO_MVM
                Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(itemName, view);
#endif
                for (size_t i = 1; i < v.size() - 1; i+=2) {
                    CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(std::string(v[i]).c_str());
                    if (attr_def == nullptr) {
                        auto idx = vi::from_str<int>(v[i]);
                        if (idx.has_value()) {
                            attr_def = GetItemSchema()->GetAttributeDefinition(*idx);
                        }
                    }

                    if (attr_def != nullptr) {
                        view->GetAttributeList().AddStringAttribute(attr_def, std::string(v[i+1]).c_str());
                    }
                }
                player->PlayTauntSceneFromItem(view);
                CEconItemView::Destroy(view);
        }},
        {"Taunt"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player{ToTFPlayer(ent)};
                player->Taunt(TAUNT_BASE_WEAPON, 0);
        }},
        {"Stun"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                Value.Convert(FIELD_FLOAT);
                CTFPlayer* player = ToTFPlayer(ent);
                player->m_Shared->StunPlayer(Value.Float(), 1.0f, TF_STUNFLAG_BONKSTUCK, ToTFPlayer(pActivator));
        }},
        {"Slowdown"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
                CTFPlayer* player = ToTFPlayer(ent);
                const std::string_view input{Value.String()};
                const auto v{vi::split_str(input, "|")};
                auto slowdown = vi::from_str<float>(v[0]);
                auto duration = vi::from_str<float>(v[1]);
                player->m_Shared->StunPlayer(duration.value_or(9999.0f), slowdown.value_or(1.0f), TF_STUNFLAG_SLOWDOWN, ToTFPlayer(pActivator));
        }}
    });

    ClassnameFilter point_viewcontrol_filter("point_viewcontrol", {
        {"EnableAll"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto camera = static_cast<CTriggerCamera *>(ent);
            ForEachTFPlayer([&](CTFPlayer *player) {
                if (player->IsBot())
                    return;
                else {

                    camera->m_hPlayer = player;
                    camera->Enable();
                    camera->m_spawnflags |= 512;
                }
            });
        }},
        {"DisableAll"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto camera = static_cast<CTriggerCamera *>(ent);
            ForEachTFPlayer([&](CTFPlayer *player) {
                if (player->IsBot())
                    return;
                else {
                    if (player->m_hViewEntity == camera) {
                        camera->m_hPlayer = player;
                        camera->Disable();
                    }
                    camera->m_spawnflags &= ~(512);
                }
            });
        }},
        {"SetTarget"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto camera = static_cast<CTriggerCamera *>(ent);
            camera->m_hTarget = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
        }}
    });
    
    ClassnameFilter trigger_detector_filter("$trigger_detector", {
        {"targettest"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto data = GetExtraTriggerDetectorData(ent);
            if (data->m_hLastTarget != nullptr) {
                ent->FireCustomOutput<"targettestpass">(data->m_hLastTarget, ent, Value);
            }
            else {
                ent->FireCustomOutput<"targettestfail">(nullptr, ent, Value);
            }
        }}
    });

    ClassnameFilter weapon_spawner_filter("$weapon_spawner", {
        {"DropWeapon"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto data = GetExtraData<ExtraEntityDataWeaponSpawner>(ent);
                auto name = ent->GetCustomVariable<"item">();
                auto item_def = GetItemSchema()->GetItemDefinitionByName(name);

                if (item_def != nullptr) {
                    auto item = CEconItemView::Create();
                    item->Init(item_def->m_iItemDefIndex, item_def->m_iItemQuality, 9999, 0);
                    item->m_iItemID = (RandomInt(INT_MIN, INT_MAX) << 16) + ENTINDEX(ent);
#ifndef NO_MVM
                    Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(name, item);
#endif
                    auto &vars = GetCustomVariables(ent);
                    for (auto &var : vars) {
                        auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(STRING(var.key));
                        if (attr_def != nullptr) {
                            item->GetAttributeList().AddStringAttribute(attr_def, var.value.String());
                        }
                    }
                    auto weapon = CTFDroppedWeapon::Create(nullptr, ent->EyePosition(), vec3_angle, item->GetPlayerDisplayModel(1, 2), item);
                    if (weapon != nullptr) {
                        if (weapon->VPhysicsGetObject() != nullptr) {
                            weapon->VPhysicsGetObject()->SetMass(25.0f);

                            if (ent->GetCustomVariableFloat<"nomotion">() != 0) {
                                weapon->VPhysicsGetObject()->EnableMotion(false);
                            }
                        }
                        auto weapondata = weapon->GetOrCreateEntityModule<DroppedWeaponModule>("droppedweapon");
                        weapondata->m_hWeaponSpawner = ent;
                        weapondata->ammo = ent->GetCustomVariableFloat<"ammo">(-1);
                        weapondata->clip = ent->GetCustomVariableFloat<"clip">(-1);
                        weapondata->energy = ent->GetCustomVariableFloat<"energy">(FLT_MIN);
                        weapondata->charge = ent->GetCustomVariableFloat<"charge">(FLT_MAX);
                        weapon->SetNextThink(gpGlobals->curtime + ent->GetCustomVariableFloat<"lifetime">(30), "RemoveThink");

                        data->m_SpawnedWeapons.push_back(weapon);
                    }
                    CEconItemView::Destroy(item);
                }
        }},
        {"RemoveDroppedWeapons"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
           
                auto data = GetExtraWeaponSpawnerData(ent);
                for (auto weapon : data->m_SpawnedWeapons) {
                    if (weapon != nullptr) {
                        weapon->Remove();
                    }
                }
                data->m_SpawnedWeapons.clear();
        }}
    });
    ClassnameFilter prop_vehicle_driveable_filter("prop_vehicle_driveable", {
        {"EnterVehicle"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto target = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            auto vehicle = rtti_cast<CPropVehicleDriveable *>(ent);
            if (ToTFPlayer(target) != nullptr && vehicle != nullptr) {
                
                Vector delta = target->GetAbsOrigin() - ent->GetAbsOrigin();
                
                QAngle angToTarget;
                VectorAngles(delta, angToTarget);
                ToTFPlayer(target)->SnapEyeAngles(angToTarget);
                
                CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
                serverVehicle->HandlePassengerEntry(ToTFPlayer(target), true);
            }
        }},
        {"ExitVehicle"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto vehicle = rtti_cast<CPropVehicleDriveable *>(ent);
            if (vehicle != nullptr && vehicle->m_hPlayer != nullptr) {
                CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
                serverVehicle->HandlePassengerExit(vehicle->m_hPlayer);
            }
        }}
    });
    ClassnameFilter script_manager_filter("$script_manager", {
        {"ExecuteScript"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto scriptManager = ent->GetOrCreateEntityModule<ScriptModule>("script");
            scriptManager->DoString(Value.String(), true);
            scriptManager->popInit = true; 
        }},
        {"ExecuteScriptFile"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto scriptManager = ent->GetOrCreateEntityModule<ScriptModule>("script");
            scriptManager->DoFile(Value.String(), true);
            scriptManager->popInit = true;
        }},
        {"Reset"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->AddEntityModule( "script", new ScriptModule(ent));
        }},
        {""sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto scriptManager = ent->GetOrCreateEntityModule<ScriptModule>("script");
            auto state = scriptManager->GetState();
            lua_getglobal(state, szInputName);
            Util::Lua::LFromVariant(state, Value);
            Util::Lua::LEntityAlloc(state, pActivator);
            Util::Lua::LEntityAlloc(state, pCaller);
            scriptManager->Call(3, 0);
            lua_settop(state, 0);
        }}
    });
    ClassnameFilter math_vector_filter("$math_vector", {
        {"Set"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            Value.Convert(FIELD_VECTOR);
            ent->SetCustomVariable("value", Value);
        }},
        {"Add"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            vec += vecValue;
            Value.SetVector3D(vec);
            ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"AddScalar"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            Value.Convert(FIELD_FLOAT);
            Vector vec = vecValue + Vector(Value.Float(), Value.Float(), Value.Float());
            Value.SetVector3D(vec);
            ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"Subtract"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            vec -= vecValue;
            Value.SetVector3D(vec);
            ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"SubtractScalar"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            Value.Convert(FIELD_FLOAT);
            Vector vec = vecValue - Vector(Value.Float(), Value.Float(), Value.Float());
            Value.SetVector3D(vec);
            ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"Multiply"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec *= vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"MultiplyScalar"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue * Value.Float();
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"Divide"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec /= vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"DivideScalar"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue / Value.Float();
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"DotProduct"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Value.SetFloat(DotProduct(vec, vecValue));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
        }},
        {"CrossProduct"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Vector out;
                    CrossProduct(vec, vecValue, out);
                    Value.SetVector3D(out);
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
        }},
        {"Distance"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Value.SetFloat(vec.DistTo(vecValue));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
        }},
        {"DistanceToEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                auto target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    Value.SetFloat(vecValue.DistTo(target->GetAbsOrigin()));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
        }},
        {"DistanceToEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.Convert(FIELD_VECTOR);
                QAngle ang;
                Value.Vector3D(*reinterpret_cast<Vector *>(&ang));
                Vector out;
                VectorRotate(vecValue, ang, out);
                Value.SetVector3D(out);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"DistanceToEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.SetFloat(vecValue.Length());
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"DistanceToEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                QAngle out;
                VectorAngles(vecValue, out);
                Value.SetVector3D(*reinterpret_cast<Vector *>(&out));
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"ToForwardVector"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Vector out;
                AngleVectors(*reinterpret_cast<QAngle *>(&vecValue), &out);
                Value.SetVector3D(out);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
        }},
        {"GetX"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.SetFloat(vecValue.x);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
        }},
        {"GetY"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.SetFloat(vecValue.y);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
        }},
        {"GetZ"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
                Value.SetFloat(vecValue.z);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
        }}
    });
    
    InputFilter input_filter({
        {"FireUserAsActivator1"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->m_OnUser1->FireOutput(Value, ent, ent);
        }},
        {"FireUserAsActivator2"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->m_OnUser2->FireOutput(Value, ent, ent);
        }},
        {"FireUserAsActivator3"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->m_OnUser3->FireOutput(Value, ent, ent);
        }},
        {"FireUserAsActivator4"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->m_OnUser4->FireOutput(Value, ent, ent);
        }},
        {"FireUser5"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->FireCustomOutput<"onuser5">(pActivator, ent, Value);
        }},
        {"FireUser6"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->FireCustomOutput<"onuser6">(pActivator, ent, Value);
        }},
        {"FireUser7"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->FireCustomOutput<"onuser7">(pActivator, ent, Value);
        }},
        {"FireUser8"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->FireCustomOutput<"onuser8">(pActivator, ent, Value);
        }},
        {"TakeDamage"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int damage = Value.Int();
            CBaseEntity *attacker = ent;
            
            CTakeDamageInfo info(attacker, attacker, nullptr, vec3_origin, ent->GetAbsOrigin(), damage, DMG_PREVENT_PHYSICS_FORCE, 0 );
            ent->TakeDamage(info);
        }},
        {"AddHealth"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            CBaseEntity *attacker = ent;
            ent->TakeHealth(Value.Int(), DMG_GENERIC);
        }},
        {"TakeDamageFromActivator"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_INTEGER);
            int damage = Value.Int();
            CBaseEntity *attacker = pActivator;
            
            CTakeDamageInfo info(attacker, attacker, nullptr, vec3_origin, ent->GetAbsOrigin(), damage, DMG_PREVENT_PHYSICS_FORCE, 0 );
            ent->TakeDamage(info);
        }},
        {"SetModelOverride"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            int replace_model = CBaseEntity::PrecacheModel(Value.String());
            if (replace_model != -1) {
                for (int i = 0; i < MAX_VISION_MODES; ++i) {
                    ent->SetModelIndexOverride(i, replace_model);
                }
            }
        }},
        {"SetModel"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CBaseEntity::PrecacheModel(Value.String());
            ent->SetModel(Value.String());
        }},
        {"SetModelSpecial"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            int replace_model = CBaseEntity::PrecacheModel(Value.String());
            if (replace_model != -1) {
                ent->SetModelIndex(replace_model);
            }
        }},
        {"SetOwner"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto owner = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetOwnerEntity(owner);
            }
        }},
        {"ClearOwner"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->SetOwnerEntity(nullptr);
        }},
        {"GetKeyValue"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            variant_t variant;
            ent->ReadKeyField(Value.String(), &variant);
            ent->m_OnUser1->FireOutput(variant, pActivator, ent);
        }},
        {"InheritOwner"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto owner = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetOwnerEntity(owner->GetOwnerEntity());
            }
        }},
        {"InheritParent"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto owner = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetParent(owner->GetMoveParent(), -1);
            }
        }},
        {"MoveType"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            variant_t variant;
            int val1=0;
            int val2=MOVECOLLIDE_DEFAULT;

            sscanf(Value.String(), "%d,%d", &val1, &val2);
            ent->SetMoveType((MoveType_t)val1, (MoveCollide_t)val2);
        }},
        {"PlaySound"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
            if (!enginesound->PrecacheSound(Value.String(), true))
                CBaseEntity::PrecacheScriptSound(Value.String());

            ent->EmitSound(Value.String());
        }},
        {"StopSound"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->StopSound(Value.String());
        }},
        {"SetLocalOrigin"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            ent->SetLocalOrigin(vec);
        }},
        {"SetLocalAngles"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_VECTOR);
            QAngle vec;
            Value.Vector3D(*reinterpret_cast<Vector *>(&vec));
            ent->SetLocalAngles(vec);
        }},
        {"SetLocalVelocity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            ent->SetLocalVelocity(vec);
        }},
        {"TeleportToEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto target = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (target != nullptr) {
                Vector targetpos = target->GetAbsOrigin();
                ent->Teleport(&targetpos, nullptr, nullptr);
            }
        }},
        {"MoveRelative"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            vec = vec + ent->GetLocalOrigin();
            ent->SetLocalOrigin(vec);
        }},
        {"RotateRelative"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Value.Convert(FIELD_VECTOR);
            QAngle vec;
            Value.Vector3D(*reinterpret_cast<Vector *>(&vec));
            vec = vec + ent->GetLocalAngles();
            ent->SetLocalAngles(vec);
        }},
        {"TestEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                auto filter = rtti_cast<CBaseFilter *>(ent);
                if (filter != nullptr && filter->PassesFilter(pCaller, target)) {
                    filter->m_OnPass->FireOutput(Value, pActivator, target);
                }
            }
        }},
        {"StartTouchEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto filter = rtti_cast<CBaseTrigger *>(ent);
            if (filter != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    filter->StartTouch(target);
                }
            }
        }},
        {"EndTouchEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto filter = rtti_cast<CBaseTrigger *>(ent);
            if (filter != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    filter->EndTouch(target);
                }
            }
        }},
        {"RotateTowards"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto rotating = rtti_cast<CFuncRotating *>(ent);
            if (rotating != nullptr) {
                CBaseEntity *target = FindTargetFromVariant(Value, ent, pActivator, pCaller);
                if (target != nullptr) {
                    auto data = GetExtraFuncRotatingData(rotating);
                    data->m_hRotateTarget = target;

                    if (rotating->GetNextThink("RotatingFollowEntity") < gpGlobals->curtime) {
                        THINK_FUNC_SET(rotating, RotatingFollowEntity, gpGlobals->curtime + 0.1);
                    }
                }
            }
            auto data = ent->GetEntityModule<RotatorModule>("rotator");
            if (data != nullptr) {
                CBaseEntity *target = FindTargetFromVariant(Value, ent, pActivator, pCaller);
                if (target != nullptr) {
                    data->m_hRotateTarget = target;

                    if (ent->GetNextThink("RotatingFollowEntity") < gpGlobals->curtime) {
                        Msg("install rotator\n");
                        THINK_FUNC_SET(ent, RotatorModuleTick, gpGlobals->curtime + 0.1);
                    }
                }
            }
        }},
        {"StopRotateTowards"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto data = ent->GetEntityModule<RotatorModule>("rotator");
            if (data != nullptr) {
                data->m_hRotateTarget = nullptr;
            }
        }},
        {"SetForwardVelocity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            Vector fwd;
            AngleVectors(ent->GetAbsAngles(), &fwd);
            fwd *= strtof(Value.String(), nullptr);

            IPhysicsObject *pPhysicsObject = ent->VPhysicsGetObject();
            if (pPhysicsObject) {
                pPhysicsObject->SetVelocity(&fwd, nullptr);
            }
            else {
                ent->SetAbsVelocity(fwd);
            }
        }},
        {"FaceEntity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            CBaseEntity *target = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            if (target != nullptr) {
                Vector delta = target->GetAbsOrigin() - ent->GetAbsOrigin();
                
                QAngle angToTarget;
                VectorAngles(delta, angToTarget);
                ent->SetAbsAngles(angToTarget);
                if (ToTFPlayer(ent) != nullptr) {
                    ToTFPlayer(ent)->SnapEyeAngles(angToTarget);
                }
            }
        }},
        {"SetFakeParent"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto data = ent->GetOrCreateEntityModule<FakeParentModule>("fakeparent");
            CBaseEntity *target = FindTargetFromVariant(Value, ent, pActivator, pCaller);
            if (target != nullptr) {
                data->m_hParent = target;
                data->m_bParentSet = true;
                if (ent->GetNextThink("FakeParentModuleTick") < gpGlobals->curtime) {
                    THINK_FUNC_SET(ent, FakeParentModuleTick, gpGlobals->curtime + 0.01);
                }
            }
        }},
        {"SetAimFollow"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
            auto data = ent->GetEntityModule<AimFollowModule>("aimfollow");
            if (data != nullptr) {
                CBaseEntity *target =  FindTargetFromVariant(Value, ent, pActivator, pCaller);
                if (target != nullptr) {
                    data->m_hParent = target;
                    if (ent->GetNextThink("AimFollowModuleTick") < gpGlobals->curtime) {
                        THINK_FUNC_SET(ent, AimFollowModuleTick, gpGlobals->curtime + 0.01);
                    }
                }
            }
        }},
        {"ClearFakeParent"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto data = ent->GetEntityModule<FakeParentModule>("fakeparent");
            if (data != nullptr) {
                data->m_hParent = nullptr;
                data->m_bParentSet = false;
            }
        }},
        {"SetVar$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            SetEntityVariable(ent, VARIABLE, szInputName + strlen("SetVar$"), Value);
        }},
        {"GetVar$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            FireGetInput(ent, VARIABLE, szInputName + strlen("GetVar$"), pActivator, pCaller, Value);
        }},
        {"SetKey$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            SetEntityVariable(ent, KEYVALUE, szInputName + strlen("SetKey$"), Value);
        }},
        {"GetKey$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            FireGetInput(ent, KEYVALUE, szInputName + strlen("GetKey$"), pActivator, pCaller, Value);
        }},
        {"SetData$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            SetEntityVariable(ent, DATAMAP, szInputName + strlen("SetData$"), Value);
        }},
        {"GetData$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            FireGetInput(ent, DATAMAP, szInputName + strlen("GetData$"), pActivator, pCaller, Value);
        }},
        {"SetProp$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            SetEntityVariable(ent, SENDPROP, szInputName + strlen("SetProp$"), Value);
        }},
        {"GetProp$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            FireGetInput(ent, SENDPROP, szInputName + strlen("GetProp$"), pActivator, pCaller, Value);
        }},
        {"SetClientProp$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<FakePropModule>("fakeprop");
            mod->props[szInputName + strlen("SetClientProp$")] = {Value, Value};
        }},
        {"ResetClientProp$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<FakePropModule>("fakeprop");
            mod->props.erase(szInputName + strlen("ResetClientProp$"));
        }},
        {"GetEntIndex"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            char param_tokenized[512] = "";
            V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
            char *targetstr = strtok(param_tokenized,"|");
            char *action = strtok(NULL,"|");
            
            variant_t variable;
            variable.SetInt(ent->entindex());
            if (targetstr != nullptr && action != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                    target->AcceptInput(action, pActivator, ent, variable, 0);
                }
            }
        }},
        {"AddModule"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            AddModuleByName(ent, Value.String());
        }},
        {"RemoveModule"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->RemoveEntityModule(Value.String());
        }},
        {"RemoveOutput"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            const char *name = Value.String();
            auto datamap = ent->GetDataDescMap();
            for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
                // search through all the readable fields in the data description, looking for a match
                for (int i = 0; i < dmap->dataNumFields; i++) {
                    if ((dmap->dataDesc[i].flags & FTYPEDESC_OUTPUT) && stricmp(dmap->dataDesc[i].externalName, name) == 0) {
                        ((CBaseEntityOutput*)(((char*)ent) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]))->DeleteAllElements();
                        return;
                    }
                }
            }
            if (name[0] == '$') {
                ent->RemoveCustomOutput(name + 1);
            }
        }},
        {"CancelPending"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            g_EventQueue.GetRef().CancelEvents(ent);
        }},
        {"SetCollisionFilter"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            if (Value.FieldType() == FIELD_EHANDLE ) {
                ent->SetCustomVariable("colfilter", Value);
            }
            else {
                variant_t variant;
                variant.SetEntity(servertools->FindEntityByName(nullptr, Value.String()));
                ent->SetCustomVariable("colfilter", variant);
            }
        }},
        {"HideTo"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<VisibilityModule>("visibility");
            if (Value.FieldType() == FIELD_EHANDLE) {
                auto target = ToTFPlayer(Value.Entity().Get());
                if (target == nullptr || !target->IsRealPlayer()) return;

                auto entry = std::find(mod->hideTo.begin(), mod->hideTo.end(), target->edict());
                if (!mod->defaultHide && entry == mod->hideTo.end())
                    mod->hideTo.push_back(target->edict());
                else if (mod->defaultHide && entry != mod->hideTo.end())
                    mod->hideTo.erase(entry);
            }
            else {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    CTFPlayer *player = ToTFPlayer(target);
                    if (player == nullptr || !player->IsRealPlayer()) continue;

                    auto entry = std::find(mod->hideTo.begin(), mod->hideTo.end(), player->edict());
                    if (!mod->defaultHide && entry == mod->hideTo.end())
                        mod->hideTo.push_back(player->edict());
                    else if (mod->defaultHide && entry != mod->hideTo.end())
                        mod->hideTo.erase(entry);
                }
            }
            ent->DispatchUpdateTransmitState();
        }},
        {"ShowTo"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<VisibilityModule>("visibility");
            if (Value.FieldType() == FIELD_EHANDLE) {
                auto target = ToTFPlayer(Value.Entity().Get());
                if (target == nullptr || !target->IsRealPlayer()) return;

                auto entry = std::find(mod->hideTo.begin(), mod->hideTo.end(), target->edict());
                if (mod->defaultHide && entry == mod->hideTo.end())
                    mod->hideTo.push_back(target->edict());
                else if (!mod->defaultHide && entry != mod->hideTo.end())
                    mod->hideTo.erase(entry);
            }
            else {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    CTFPlayer *player = ToTFPlayer(target);
                    if (player == nullptr || !player->IsRealPlayer()) continue;

                    auto entry = std::find(mod->hideTo.begin(), mod->hideTo.end(), player->edict());
                    if (mod->defaultHide && entry == mod->hideTo.end())
                        mod->hideTo.push_back(player->edict());
                    else if (!mod->defaultHide && entry != mod->hideTo.end())
                        mod->hideTo.erase(entry);
                }
            }
            ent->DispatchUpdateTransmitState();
        }},
        {"HideToAll"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<VisibilityModule>("visibility");
            mod->defaultHide = true;
            mod->hideTo.clear();
            ent->DispatchUpdateTransmitState();
        }},
        {"ShowToAll"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            auto mod = ent->GetOrCreateEntityModule<VisibilityModule>("visibility");
            mod->defaultHide = false;
            mod->hideTo.clear();
            ent->DispatchUpdateTransmitState();
        }},
        {"VScriptFunc$"sv, true, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            ent->AcceptInput("RunScriptCode", pActivator, pCaller, Variant(AllocPooledString(CFmtStr(Value.FieldType() == FIELD_STRING ? "%s(\"%s\")" : "%s(%s)", szInputName + strlen("VScriptFunc$"), Value.String()))), -1);
        }},
        {"BotCommand"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            if (ent->MyNextBotPointer() != nullptr)
                ent->MyNextBotPointer()->OnCommandString(Value.String());
        }},
        {"StopParticleEffects"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            StopParticleEffects(ent);
        }}
    });
}