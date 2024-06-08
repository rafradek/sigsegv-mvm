#include "mod.h"
#include "stub/baseentity.h"
#include "stub/baseplayer.h"
#include "stub/extraentitydata.h"
#include "stub/server.h"
#include "stub/sendprop.h"
#include "stub/gamerules.h"
#include "util/prop_helper.h"
#include "util/misc.h"
#include "mod/etc/sendprop_override.h"
#include <bitset>
#include <fmt/core.h>

extern IExtensionManager *smexts;

namespace Mod::Etc::SendProp_Override_Extension
{
    struct PluginCallbackInfo
    {
        CBaseEntity *entity;
        IPluginFunction *callback; 
        int arrayIndex;
        std::string name;
        int callbackIndex;
        bool isPerClient;
        bool valueSet = false;
        bool isSendProxyExt = false;
        bool isGameRules = false;
        variant_t valuePre;
        std::bitset<ABSOLUTE_PLAYER_LIMIT+1> valueSetClients;
        std::vector<variant_t> valueApplyClients;
        PropCacheEntry propCacheEntry;
    };
    std::vector<PluginCallbackInfo> callbacks;

    struct PluginPropChangeCallback
    {
        CBaseEntity *entity;
        IPluginFunction *callback; 
        std::string name;
        PropCacheEntry propCacheEntry;
        variant_t valuePre;
    };
    std::vector<PluginPropChangeCallback> propChangeCallbacks;

    // Automatically deletes callbacks when entity is removed
    class PluginSendpropOverrideModule : public EntityModule
    {
    public:
        PluginSendpropOverrideModule(CBaseEntity *entity) : EntityModule(entity), entity(entity) {}
        ~PluginSendpropOverrideModule() {
            RemoveIf(callbacks, [this](auto &callback){
                return callback.entity == entity;
            });
            RemoveIf(propChangeCallbacks, [this](auto &callback){
                return callback.entity == entity;
            });
        }

        CBaseEntity *entity;
    };

	void CallbackPluginCall(CBaseEntity *entity, int clientIndex, DVariant &value, int callbackIndex, SendProp *prop, uintptr_t data) {
		auto &info = callbacks[data];
        if (!info.valueSet || !info.valueSetClients.test(clientIndex)) return;

        variant_t &varValue = info.valueApplyClients[clientIndex];

        switch (value.m_Type) 
        {
        case DPT_Int: {
            if (varValue.FieldType() == FIELD_EHANDLE) {
                auto handle = varValue.Entity();
                int serialNum = handle.GetSerialNumber() & ((1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1);
                value.m_Int = handle.Get() ? (handle.GetEntryIndex() | (serialNum << MAX_EDICT_BITS)) : INVALID_NETWORKED_EHANDLE_VALUE;
            }
            else {
                varValue.Convert(FIELD_INTEGER);
                value.m_Int = varValue.Int();
            }
            break;
        }
        case DPT_Float: {
            varValue.Convert(FIELD_FLOAT);
            value.m_Float = varValue.Float();
            break;
        }
        case DPT_String: {
            value.m_pString = varValue.String();
            break;
        }
        case DPT_Vector: {
            varValue.Convert(FIELD_VECTOR);
            varValue.Vector3D(*(reinterpret_cast<Vector *>(value.m_Vector)));
            break;
        }
        case DPT_VectorXY: {
            varValue.Convert(FIELD_VECTOR);
            varValue.Vector3D(*(reinterpret_cast<Vector *>(value.m_Vector)));
            break;
        }
        default:
            break;
        }
        
	}

    void RemovePluginCallbacks(IPlugin *plugin) noexcept
	{
        RemoveIf(callbacks, [plugin](auto &callback){
            if (callback.callback->GetParentContext() == plugin->GetBaseContext()) {
                CBaseEntity *entity = callback.entity;
                auto mod = entity->GetEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
                if (mod != nullptr)
                    mod->RemoveOverride(callback.callbackIndex);
                return true;
            }
            return false;
        });
	}

    static cell_t RemoveHookEntry(IPluginContext *pContext, cell_t entityParam, cell_t nameParam, cell_t callbackParam, cell_t arrayIndexParam) noexcept
    {
        char *name_ptr;
        pContext->LocalToString(nameParam, &name_ptr);
        std::string name {name_ptr};

        CBaseEntity *entity = gamehelpers->ReferenceToEntity(entityParam);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", entityParam); 
            return 0;
        }

        IPluginFunction *callback{pContext->GetFunctionById(callbackParam)};

        RemoveIf(callbacks, [entity, name, callback, arrayIndexParam](auto &info){
            if (info.callback == callback && entity == info.entity && name == info.name && (arrayIndexParam == -1 || arrayIndexParam == info.arrayIndex)) {
                CBaseEntity *entity = info.entity;
                auto mod = entity->GetEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
                if (mod != nullptr) {
                    mod->RemoveOverride(info.callbackIndex);
                }
                return true;
            }
            return false;
        });

        return 1;
    }

    static cell_t proxysend_unhook(IPluginContext *pContext, const cell_t *params) noexcept
    {
        RemoveHookEntry(pContext, params[1], params[2], params[3], -1);
        return 0;
    }
    
    static cell_t SendProxy_Unhook(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return RemoveHookEntry(pContext, params[1], params[2], params[3], -1);
    }
    
    static cell_t SendProxy_UnhookArrayProp(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return RemoveHookEntry(pContext, params[1], params[2], params[5], params[3]);
    }
    
    static cell_t SendProxy_UnhookGameRules(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return RemoveHookEntry(pContext, CGameRulesProxy::s_pGameRulesProxy.GetRef()->entindex(), params[1], params[2], -1);
    }

    bool SendProxyIsHooked(IPluginContext *pContext, cell_t entityParam, cell_t nameParam)
    {
        CBaseEntity *entity = gamehelpers->ReferenceToEntity(entityParam);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", entityParam);
            return 0;
        }
        
        char *name_ptr;
        pContext->LocalToString(nameParam, &name_ptr);
        std::string name{name_ptr};

        for (auto &entry : callbacks) {
            if (entry.isSendProxyExt && entry.entity == entity && entry.name == name) return true;
        }
        return false;
    }
    
    static cell_t SendProxy_IsHooked(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return SendProxyIsHooked(pContext, params[1], params[2]);
    }
    
    static cell_t SendProxy_IsHookedGameRules(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return SendProxyIsHooked(pContext, CGameRulesProxy::s_pGameRulesProxy.GetRef()->entindex(), params[1]);
    }
    
    cell_t AddHookEntry(CBaseEntity *entity, std::string &name, SendProp *prop, ServerClass *sclass, IPluginFunction *callback, int arrayIndex, bool perClient, IPluginContext *context, PropCacheEntry &propCacheEntry, int precalcIndex, bool isSendProxyExt, bool isGameRules)
    {
        auto mod = entity->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");// m_iTeam.SetIndex(gpGlobals->tickcount % 2, i);
        auto &callbackEntry = callbacks.emplace_back();
        callbackEntry.isPerClient = perClient;
        callbackEntry.callback = callback;
        if (perClient) {
            callbackEntry.callbackIndex = mod->AddOverride(CallbackPluginCall, precalcIndex, prop, callbacks.size() - 1);
            if (callbackEntry.callbackIndex == -1) {
                callbacks.pop_back();
                context->ReportError("Could not find prop %s", name.c_str());
                return 0;
            }
            callbackEntry.valueApplyClients.resize(gpGlobals->maxClients+1);
        }
        entity->GetOrCreateEntityModule<PluginSendpropOverrideModule>("pluginsendpropoverride");
        callbackEntry.arrayIndex = arrayIndex;
        callbackEntry.name = name;
        callbackEntry.entity = entity;
        callbackEntry.propCacheEntry = propCacheEntry;
        callbackEntry.isSendProxyExt = isSendProxyExt;
        callbackEntry.isGameRules = isGameRules;
        return 0;
    }

    cell_t AddHook(IPluginContext *pContext, cell_t entityParam, cell_t nameParam, cell_t callbackParam, cell_t perClientParam, bool isSendProxyExt, bool isGameRules)
    {
        CBaseEntity *entity = isGameRules ? CGameRulesProxy::s_pGameRulesProxy.GetRef() : gamehelpers->ReferenceToEntity(entityParam);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", entityParam);
            return 0;
        }

        char *namePtr;
        pContext->LocalToString(nameParam, &namePtr);
        std::string name = namePtr;

        IPluginFunction *callback = pContext->GetFunctionById(callbackParam);

        bool per_client = static_cast<bool>(perClientParam);

        ServerClass *sclass = entity->GetServerClass();

        PropCacheEntry &propCacheEntry = GetSendPropOffset(sclass, name);

        SendProp *prop = propCacheEntry.prop;
        if (prop == nullptr) {
            pContext->ReportError("Could not find prop %s", name.c_str());
            return 0;
        }

        auto precalc = sclass->m_pTable->m_pPrecalc;
        int precalcIndex = -1;

        if(prop->GetType() == DPT_DataTable) {
            SendTable *propTable = prop->GetDataTable();
            if (propTable->GetNumProps() <= 0) return 0;
            SendProp *childProp0 = propTable->GetProp(0);
            
            for (int i = 0; i < precalc->m_Props.Count(); i++) {
                if (precalc->m_Props[i] == childProp0) {
                    precalcIndex = i;
                    break;
                }
            }
            if (precalcIndex == -1) return 0;

            for(int i = 0; i < propTable->GetNumProps(); ++i) {
                SendProp *childProp = propTable->GetProp(i);
                
                auto result = AddHookEntry(entity, name, childProp, sclass, callback, i, per_client, pContext, propCacheEntry, precalcIndex + i, isSendProxyExt, isGameRules);
                if (result != 0) return result;
            }
            return 0;
        }

        for (int i = 0; i < precalc->m_Props.Count(); i++) {
            if (precalc->m_Props[i] == prop) {
                precalcIndex = i;
                break;
            }
        }
        if (precalcIndex == -1) {
            pContext->ReportError("Could not find prop %s", name.c_str());
            return 0;
        }

        return AddHookEntry(entity, name, prop, sclass, callback, 0, per_client, pContext, propCacheEntry, precalcIndex, isSendProxyExt, isGameRules);
    }
    
    static cell_t proxysend_hook(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return AddHook(pContext, params[1], params[2], params[3], params[4], false, false);
    }

    static cell_t SendProxy_Hook(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return AddHook(pContext, params[1], params[2], params[4], false, true, false);
    }

    static cell_t SendProxy_HookGameRules(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return AddHook(pContext, 0, params[1], params[3], false, true, true);
    }

    static cell_t SendProxy_HookArrayProp(IPluginContext *pContext, const cell_t *params) noexcept
    {
        CBaseEntity *entity = gamehelpers->ReferenceToEntity(params[1]);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", params[1]);
            return 0;
        }

        char *namePtr;
        pContext->LocalToString(params[2], &namePtr);
        int element = params[3];
        std::string name = fmt::format("{}${}", namePtr, element);
        
        IPluginFunction *callback = pContext->GetFunctionById(params[5]);

        ServerClass *sclass = entity->GetServerClass();

        PropCacheEntry propCacheEntry = GetSendPropOffset(sclass, namePtr);

        SendProp *prop = propCacheEntry.prop;
        if (prop == nullptr) {
            pContext->ReportError("Could not find prop %s", name.c_str());
            return 0;
        }

        if(prop->GetType() == DPT_DataTable) {
            SendTable *propTable = prop->GetDataTable();
            if (propTable->GetNumProps() < element) return 0;
            SendProp *childProp = propTable->GetProp(element);
            propCacheEntry.arraySize = 1;
            propCacheEntry.offset += propCacheEntry.elementStride * element;
            auto result = AddHookEntry(entity, name, childProp, sclass, callback, element, false, pContext, propCacheEntry, 0, true, false);
            if (result == 0) return 1;
        }
        return 0;
    }

    bool CallPluginFunction(PluginCallbackInfo &info, variant_t &value, int client) {
        auto func = info.callback;
        if (!(info.isSendProxyExt && info.isGameRules))
            func->PushCell(info.entity->entindex());
		func->PushStringEx((char *)info.name.c_str(), info.name.size()+1, SM_PARAM_STRING_COPY|SM_PARAM_STRING_UTF8, 0);
        switch(value.FieldType()) {
            case FIELD_FLOAT: {

                float sp_value = value.Float();
                func->PushFloatByRef(&sp_value);
                func->PushCell(info.arrayIndex);
                
                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    value.SetFloat(sp_value);
                    return true;
                }
                return false;
            }
            case FIELD_VECTOR: case FIELD_POSITION_VECTOR: {
                Vector vec;
                value.Vector3D(vec);
                cell_t sp_value[] {sp_ftoc(vec.x), sp_ftoc(vec.y), sp_ftoc(vec.z)};
                func->PushArray(sp_value, 3, SM_PARAM_COPYBACK);
                func->PushCell(info.arrayIndex);
                
                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    vec.Init(sp_ctof(sp_value[0]), sp_ctof(sp_value[1]), sp_ctof(sp_value[2]));
                    value.SetVector3D(vec);
                    return true;
                }
                return false;
            }
            case FIELD_COLOR32: {
                color32 color = value.Color32();
                cell_t spR = color.r;
                cell_t spG = color.g;
                cell_t spB = color.b;
                cell_t spA = color.a;
                func->PushCellByRef(&spR);
                func->PushCellByRef(&spG);
                func->PushCellByRef(&spB);
                func->PushCellByRef(&spA);
                func->PushCell(info.arrayIndex);

                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    value.SetColor32(spR, spG, spB, spA);
                    return true;
                }
                return false;
            }
            case FIELD_STRING: {

                static char sp_value[4096];
                strcpy(sp_value, value.String());
                func->PushStringEx(sp_value, strlen(sp_value)+1, SM_PARAM_STRING_UTF8|SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);
                if (!info.isSendProxyExt)
                    func->PushCell(sizeof(sp_value));
                func->PushCell(info.arrayIndex);

                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    value.SetString(AllocPooledString(sp_value));
                    return true;
                }
                return false;
            }
            case FIELD_EHANDLE: {
                CBaseEntity *ent = value.Entity().Get();
                cell_t sp_value = ent != nullptr ? gamehelpers->EntityToBCompatRef(ent) : -1;
                func->PushCellByRef(&sp_value);
                func->PushCell(info.arrayIndex);

                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    value.SetEntity(gamehelpers->ReferenceToEntity(sp_value));
                    return true;
                }
                return false;
            }
            default: {
                value.Convert(FIELD_INTEGER);
                cell_t sp_value = value.Int();
                func->PushCellByRef(&sp_value);
                func->PushCell(info.arrayIndex);

                if (!info.isSendProxyExt)
                    func->PushCell(client);
                cell_t res{Pl_Continue};
                func->Execute(&res);
                if(res == Pl_Changed) {
                    info.valueSet = true;
                    value.SetInt(sp_value);
                    return true;
                }
                return false;
            }
        }
		
		return false;
    }

    cell_t AddPropChangeHook(IPluginContext *pContext, cell_t entityParam, cell_t nameParam, cell_t callbackParam, bool isGameRules)
    {
        CBaseEntity *entity = isGameRules ? CGameRulesProxy::s_pGameRulesProxy.GetRef() : gamehelpers->ReferenceToEntity(entityParam);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", entityParam);
            return 0;
        }

        char *namePtr;
        pContext->LocalToString(nameParam, &namePtr);
        std::string name = namePtr;

        IPluginFunction *callback = pContext->GetFunctionById(callbackParam);

        ServerClass *sclass = entity->GetServerClass();

        PropCacheEntry &propCacheEntry = GetSendPropOffset(sclass, name);

        SendProp *prop = propCacheEntry.prop;
        if (prop == nullptr) {
            pContext->ReportError("Could not find prop %s", name.c_str());
            return 0;
        }

        variant_t value;
        ReadProp(entity, propCacheEntry, value, 0, -1);
        propChangeCallbacks.push_back({entity, callback, name, propCacheEntry, value});
        entity->GetOrCreateEntityModule<PluginSendpropOverrideModule>("pluginsendpropoverride");
        return 1;
    }

    static cell_t SendProxy_HookPropChange(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return AddPropChangeHook(pContext, params[1], params[2], params[3], false);
    }

    static cell_t SendProxy_HookPropChangeGameRules(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return AddPropChangeHook(pContext, 0, params[1], params[2], true);
    }

    cell_t RemovePropChangeHook(IPluginContext *pContext, cell_t entityParam, cell_t nameParam, cell_t callbackParam, bool isGameRules)
    {
        CBaseEntity *entity = isGameRules ? CGameRulesProxy::s_pGameRulesProxy.GetRef() : gamehelpers->ReferenceToEntity(entityParam);
        if(!entity) {
            pContext->ReportError("Invalid Entity Reference/Index %i", entityParam);
            return 0;
        }

        char *namePtr;
        pContext->LocalToString(nameParam, &namePtr);
        std::string name = namePtr;

        IPluginFunction *callback = pContext->GetFunctionById(callbackParam);

        RemoveIf(propChangeCallbacks, [&](PluginPropChangeCallback &entry){
            return entry.callback == callback && entry.name == name && entry.entity == entity;
        });
        return 1;
    }

    static cell_t SendProxy_UnhookPropChange(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return RemovePropChangeHook(pContext, params[1], params[2], params[3], false);
    }

    static cell_t SendProxy_UnhookPropChangeGameRules(IPluginContext *pContext, const cell_t *params) noexcept
    {
        return RemovePropChangeHook(pContext, 0, params[1], params[2], true);
    }

    DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount, CGameClient **clients, void *snapshot)
	{
		for (auto &plCallback : callbacks) {
            plCallback.valueSet = false;
            variant_t value;
            ReadProp(plCallback.entity, plCallback.propCacheEntry, value, plCallback.arrayIndex, -1);
            plCallback.valuePre = value;
            
            if (!plCallback.isPerClient) {
                if (CallPluginFunction(plCallback, value, 0)) {
                    WriteProp(plCallback.entity, plCallback.propCacheEntry, value, plCallback.arrayIndex, -1);
                    plCallback.entity->NetworkStateChanged();
                }
            }
            else {
                for (int i = 0; i < clientCount; i++) {
                    auto client = clients[i];
                    auto num = client->m_nEntityIndex;

                    if (CallPluginFunction(plCallback, value, num)) {
                        plCallback.valueSetClients.set(num);
                    }
                    plCallback.valueApplyClients[num] = value;
                }
            }
        }
		DETOUR_STATIC_CALL(clientCount, clients, snapshot);
		for (auto &plCallback : callbacks) {
            if (plCallback.isPerClient) {
                if (plCallback.valueSet) {
                    plCallback.valueSetClients.reset();
                }
            }
            if (plCallback.valueSet && !plCallback.isPerClient) {
                WriteProp(plCallback.entity, plCallback.propCacheEntry, plCallback.valuePre, plCallback.arrayIndex, -1);
                plCallback.entity->NetworkStateChanged();
            }
        }
	}

    static constexpr const sp_nativeinfo_t natives[]{
        {"proxysend_hook", proxysend_hook},
        {"proxysend_unhook", proxysend_unhook},
        {nullptr, nullptr}
    };

    IExtension *extension = nullptr;
    class CProxysendExtension :
	public IExtensionInterface, IPluginsListener
    {
        virtual bool OnExtensionLoad(IExtension *me,
			IShareSys *sys, 
			char *error, 
			size_t maxlength, 
			bool late) override { 
                sharesys->AddNatives(me, natives);
                plsys->AddPluginsListener(this);
                return true; 
            }

		virtual void OnExtensionUnload() override {
	        plsys->RemovePluginsListener(this);
        }

		virtual void OnExtensionsAllLoaded() override {}

		virtual void OnExtensionPauseChange(bool pause) override {}

        virtual void OnPluginUnloaded(IPlugin *plugin) override { RemovePluginCallbacks(plugin); }

	public:
		virtual bool IsMetamodExtension() { return false; }

		virtual const char *GetExtensionName() { return "proxysend"; }
		virtual const char *GetExtensionURL() { return ""; }
		virtual const char *GetExtensionVerString() { return "sigsegv"; }
		virtual const char *GetExtensionAuthor() { return "rafradek"; }
		virtual const char *GetExtensionDescription() { return "proxysend provided by sigsegv extension"; }
		virtual const char *GetExtensionTag() { return "PROXYSEND"; }
		virtual const char *GetExtensionDateString() { return __DATE__; }
    };
    CProxysendExtension proxysendExtension;

    static constexpr const sp_nativeinfo_t natives_sendproxy[]{
        {"SendProxy_Hook", SendProxy_Hook},
        {"SendProxy_HookGameRules", SendProxy_HookGameRules},
        {"SendProxy_HookArrayProp", SendProxy_HookArrayProp},
        {"SendProxy_UnhookArrayProp", SendProxy_UnhookArrayProp},
        {"SendProxy_Unhook", SendProxy_Unhook},
        {"SendProxy_UnhookGameRules", SendProxy_UnhookGameRules},
        {"SendProxy_IsHooked", SendProxy_IsHooked},
        {"SendProxy_IsHookedGameRules", SendProxy_IsHooked},
        {"SendProxy_HookPropChange", SendProxy_HookPropChange},
        {"SendProxy_HookPropChangeGameRules", SendProxy_HookPropChangeGameRules},
        {"SendProxy_UnhookPropChange", SendProxy_UnhookPropChange},
        {"SendProxy_UnhookPropChangeGameRules", SendProxy_UnhookPropChangeGameRules},
        {nullptr, nullptr}
    };


    IExtension *extensionSendproxy = nullptr;
    class CSendproxyExtension :
	public IExtensionInterface, IPluginsListener
    {
        virtual bool OnExtensionLoad(IExtension *me,
			IShareSys *sys, 
			char *error, 
			size_t maxlength, 
			bool late) override { 
                sharesys->AddNatives(me, natives_sendproxy);
                plsys->AddPluginsListener(this);
                return true; 
            }

		virtual void OnExtensionUnload() override {
	        plsys->RemovePluginsListener(this);
        }

		virtual void OnExtensionsAllLoaded() override {}

		virtual void OnExtensionPauseChange(bool pause) override {}

        virtual void OnPluginUnloaded(IPlugin *plugin) override { RemovePluginCallbacks(plugin); }

	public:
		virtual bool IsMetamodExtension() { return false; }

		virtual const char *GetExtensionName() { return "SendProxy Manager"; }
		virtual const char *GetExtensionURL() { return "http://www.afronanny.org/"; }
		virtual const char *GetExtensionVerString() { return "sigsegv"; }
		virtual const char *GetExtensionAuthor() { return "rafradek"; }
		virtual const char *GetExtensionDescription() { return "sendproxy provided by sigsegv extension"; }
		virtual const char *GetExtensionTag() { return "SENDPROXY"; }
		virtual const char *GetExtensionDateString() { return __DATE__; }
    };
    CSendproxyExtension sendproxyExtension;

	class CMod : public IMod, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:SendProp_Override_Extension")
		{
            MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks, "SV_ComputeClientPacks");
        }
        virtual void OnEnable() override {
            char err[256];
            extension = smexts->LoadExternal(&proxysendExtension, "proxysend", "proxysend.ext", err, 256);
            extensionSendproxy = smexts->LoadExternal(&sendproxyExtension, "SendProxy Manager", "sendproxy.ext", err, 256);
            if (extension == nullptr) {
                ConColorMsg(Color(0xff, 0x00, 0x00), "[Sigsegv-MvM] Proxysend extension cannot be loaded before this extension. This will cause issues\n");
            }
            if (extensionSendproxy == nullptr) {
                ConColorMsg(Color(0xff, 0x00, 0x00), "[Sigsegv-MvM] SendProxy Manager extension cannot be loaded before this extension. This will cause issues\n");
            }
        }
        virtual void OnDisable() override {
            if (extension != nullptr)
                smexts->UnloadExtension(extension);
            if (extensionSendproxy != nullptr)
                smexts->UnloadExtension(extensionSendproxy);
        }

        virtual bool EnableByDefault() override { 
            char path_sm[PLATFORM_MAX_PATH];
            g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"extensions/proxysend.ext*");
		    FileFindHandle_t fileH;
            for (const char *fileName = filesystem->FindFirstEx(path_sm, "GAME", &fileH);
							fileName != nullptr; fileName = filesystem->FindNext(fileH)) {
		        filesystem->FindClose(fileH);
                return true;
			}
		    filesystem->FindClose(fileH);
            g_pSM->BuildPath(Path_SM,path_sm,sizeof(path_sm),"extensions/sendproxy.ext*");
            for (const char *fileName = filesystem->FindFirstEx(path_sm, "GAME", &fileH);
							fileName != nullptr; fileName = filesystem->FindNext(fileH)) {
		        filesystem->FindClose(fileH);
                return true;
			}
		    filesystem->FindClose(fileH);
            return false;
        }

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled() && !propChangeCallbacks.empty(); }

        virtual void FrameUpdatePostEntityThink() override
        {
            for (auto &entry : propChangeCallbacks) {
                variant_t value;
                ReadProp(entry.entity, entry.propCacheEntry, value, 0, -1);
                if (entry.valuePre != value) {
                    entry.callback->PushString(entry.name.c_str());
                    std::string oldVal = entry.valuePre.FieldType() == FIELD_EHANDLE ? std::to_string(entry.valuePre.Entity().ToInt()) : entry.valuePre.String();
                    std::string newVal = value.FieldType() == FIELD_EHANDLE ? std::to_string(value.Entity().ToInt()) : value.String();
                    entry.callback->PushString(oldVal.c_str());
                    entry.callback->PushString(newVal.c_str());
                    entry.callback->Execute(0);
                    entry.valuePre = value;
                }
            }
        }

		virtual std::vector<std::string> GetRequiredMods() { return {"Etc:SendProp_Override"};}
    };
	CMod s_Mod;
    
	
	ConVar cvar_enable("sig_etc_sendprop_override_extension", "0", FCVAR_NOTIFY,
		"Mod: Force load buildin proxysend/sendproxy extension. Loaded automatically when proxysend extension is detected in extensions",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}