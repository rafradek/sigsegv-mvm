#include "mod.h"
#include "stub/extraentitydata.h"
#include "stub/usermessages_sv.h"
#include "stub/misc.h"
#include "util/misc.h"

class HudTextDisplayModule : public EntityModule
{
public:
    HudTextDisplayModule() : EntityModule() {}
    HudTextDisplayModule(CBaseEntity *entity) : EntityModule(entity) {}

    float fadeoutTime[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    int internalId[6] = {0, 0, 0, 0, 0, 0};
};

void DisplayHudMessageAutoChannel(CBasePlayer *player, hudtextparms_t & params, const char *message, int internalId)
{
    if (player == nullptr) {
        UTIL_HudMessage(player, params, message);
        return;
    }
    auto mod = player->GetEntityModule<HudTextDisplayModule>("huddisplaymodule");
    if (mod != nullptr) {
        if (mod->fadeoutTime[params.channel % 6] > gpGlobals->curtime && mod->internalId[params.channel % 6] != internalId) {
            bool found = false;
            // Check for already used internalId
            for (int i = 0; i < 6; i++) {
                if (mod->internalId[i] == internalId) {
                    params.channel = i;
                    found = true;
                }
            }
            // Check for oldest text
            if (!found) {
                float oldestDisplayTime = FLT_MAX;
                int oldestChannel = -1;
                for (int i = 0; i < 6; i++) {
                    if (mod->fadeoutTime[i] < oldestDisplayTime) {
                        oldestDisplayTime = mod->fadeoutTime[i];
                        oldestChannel = i;
                    }
                }
                params.channel = oldestChannel;
            }
        }
    }
    UTIL_HudMessage(player, params, message);
    if (mod != nullptr) {
        mod->internalId[params.channel % 6] = internalId;
    }
}

namespace Mod::Common::Text_Hud
{
    int hudmsg_id = 0;

    class HudMsgListener : public IBitBufUserMessageListener
	{
	public:

		virtual void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
		{
            auto ptr = bf->GetBasePointer();
            int channel = *(int8_t *)ptr;
            int effect = *((char *)ptr + 1 + 2 * 4 + 8);
            float fadein = *(float *)((char *)ptr + 1 + 2 * 4 + 8 + 1);
            float fadeout = *(float *)((char *)ptr + 1 + 2 * 4 + 8 + 1 + 4);
            float holdtime = *(float *)((char *)ptr + 1 + 2 * 4 + 8 + 1 + 8);
            
            for (int i = 0; i < pFilter->GetRecipientCount(); i++) {
                auto player = UTIL_PlayerByIndex(pFilter->GetRecipientIndex(i));
                if (player != nullptr) {
                    auto mod = player->GetOrCreateEntityModule<HudTextDisplayModule>("huddisplaymodule");
                    mod->fadeoutTime[channel % 6] = gpGlobals->curtime + fadein + holdtime + fadeout;
                    mod->internalId[channel % 6] = 0;
                }
            }
		}
	};
    
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Common:Text_Hud")
		{
		}

        virtual void OnEnable() override { 
			usermsgs->HookUserMessage2(usermsgs->GetMessageIndex("HudMsg"), &m_HudMsgListener);
        }

        virtual void OnDisable() override { 
			usermsgs->UnhookUserMessage2(usermsgs->GetMessageIndex("HudMsg"), &m_HudMsgListener);
        }

        virtual bool EnableByDefault() override { return true; }

    private:
        HudMsgListener m_HudMsgListener;
	};
	CMod s_Mod;
}