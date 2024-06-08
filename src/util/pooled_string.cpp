#include "util/pooled_string.h"
#include "mod.h"

namespace Util::PooledStringMod
{
    DETOUR_DECL_MEMBER(void, CGameStringPool_LevelShutdownPostEntity)
    {
        DETOUR_MEMBER_CALL();
        for (auto &string : PooledString::List()) {
            string->Reset();
        }
    }


    class CMod : public IMod
    {
    public:
        CMod() : IMod("Util:Pooled_String")
        {
            MOD_ADD_DETOUR_MEMBER(CGameStringPool_LevelShutdownPostEntity, "CGameStringPool::LevelShutdownPostEntity");
        }
        virtual bool OnLoad() override
        {
            for (auto &string : PooledString::List()) {
                string->Reset();
            }
            return true;
        }

        virtual bool EnableByDefault() override { return true; }
    };
    CMod s_Mod;
}

