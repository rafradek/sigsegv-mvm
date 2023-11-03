#ifndef _INCLUDE_SIGSEGV_MOD_ETC_SENDPROP_OVERRIDE_H_
#define _INCLUDE_SIGSEGV_MOD_ETC_SENDPROP_OVERRIDE_H_

namespace Mod::Etc::SendProp_Override
{
    typedef void ( *SendPropOverrideCallback )(CBaseEntity *, int, DVariant &, int, SendProp *, uintptr_t data);

    struct PropOverride
    {
        int id;
        SendPropOverrideCallback callback;
        SendProp *prop;
        int propIndex;
        uintptr_t data;
        bool stopped = false;
    };


    class SendpropOverrideModule : public EntityModule, public AutoList<SendpropOverrideModule>
	{
	public:
		SendpropOverrideModule(CBaseEntity *entity) : EntityModule(entity), entity(entity) {}

        ~SendpropOverrideModule();

        int AddOverride(SendPropOverrideCallback callback, const std::string &name, int index = -1, uintptr_t data = 0);
        int AddOverride(SendPropOverrideCallback callback, int indexProp, SendProp *prop, uintptr_t data = 0);
        
        void RemoveOverride(int id);
        
        CBaseEntity *entity;
        std::vector<PropOverride> propOverrides;

	};
}
#endif