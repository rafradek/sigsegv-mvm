
#include "util/misc.h"
#include "stub/misc.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/tfweaponbase.h"
#include "mod/pop/common.h"
#include "util/iterate.h"
#include "util/lua.h"
#include "util/admin.h"
#include "util/clientmsg.h"
#include "mod/etc/mapentity_additions.h"
#include "mod.h"

class CStaticProp {};

namespace Util::Lua
{
    LuaState *cur_state = nullptr;

    int vector_meta = LUA_NOREF;
    const void *vector_meta_ptr = nullptr;
    
    int entity_meta = LUA_NOREF;
    const void *entity_meta_ptr = nullptr;

    int prop_meta = LUA_NOREF;
    int entity_table_store = LUA_NOREF;

    static void stackDump (lua_State *L) {
      int i;
      int top = lua_gettop(L);
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        printf("%d: ", i);
        switch (t) {
    
          case LUA_TSTRING:  /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
    
          case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
    
          default:  /* other values */
            printf("%s", lua_typename(L, t));
            break;
    
        }
        printf("\n");  /* put a separator */
      }
      printf("\n");  /* end the listing */
    }

    void ConstructPrint(lua_State *l, int index, std::string &str)
    {
        int args = lua_gettop(l);
        for (int i = index; i <= args; i++) {
            const char *msg = nullptr;
            auto type = lua_type(l, i);
            if (type == LUA_TSTRING || type == LUA_TNUMBER) {
                msg = lua_tostring(l, i);
            }
            else {
                lua_getglobal(l, "tostring");
                //lua_pushvalue(l, -1);
                lua_pushvalue(l,i);
                lua_pcall(l, 1, -2, 0);
                msg = lua_tostring(l, -1);
            }
            str += msg != nullptr ? msg : "";
            str += '\t';
        }
    }

    int LPrint(lua_State *l)
    {
        std::string str = "";
        ConstructPrint(l, 1, str);
        SendConsoleMessageToAdmins("%s\n", str.c_str());
        Msg("%s\n", str.c_str());
        return 0;
    }

    inline QAngle *LAngleAlloc(lua_State *l)
    {
        QAngle *angles = (QAngle *)lua_newuserdata(l, sizeof(Vector));
        lua_rawgeti(l, LUA_REGISTRYINDEX, vector_meta);
        lua_setmetatable(l, -2);
        return angles;
    }

    inline Vector *LVectorAlloc(lua_State *l)
    {
        Vector *vec = (Vector *)lua_newuserdata(l, sizeof(Vector));
        lua_rawgeti(l, LUA_REGISTRYINDEX, vector_meta);
        lua_setmetatable(l, -2);
        return vec;
    }

    int LVectorNew(lua_State *l)
    {
        if (lua_type(l, 1) == LUA_TSTRING) {
            auto vec = LVectorAlloc(l);
            UTIL_StringToVectorAlt(*vec, lua_tostring(l, 1));
        }
        else {
            float x = luaL_checknumber(l, 1);
            float y = luaL_checknumber(l, 2);
            float z = luaL_checknumber(l, 3);

            LVectorAlloc(l)->Init(x, y, z);
        }
        return 1;
    }

    // Faster version of userdata check that compares metadata pointers
    inline void *LGetUserdata(lua_State *l, int index, const void *metatablePtr)
    {
        if (lua_getmetatable(l, index)) {
            auto ourMetatablePtr = lua_topointer(l, -1);
            lua_pop(l,1);
            if (metatablePtr == ourMetatablePtr) {
                void *ud = lua_touserdata(l, index);
                if (ud != nullptr) { 
                    return ud;
                }
            }
        }
        return nullptr;
    }

    // Faster version of userdata check that compares metadata pointers
    inline void *LCheckUserdata(lua_State *l, int index, const void *metatablePtr, const char *name)
    {
        auto ud = LGetUserdata(l, index, metatablePtr);
        if (ud == nullptr) {
            luaL_error(l, "Invalid argument %d: expected '%s'", index, name);
        }
        return ud;
    }

    inline Vector *LVectorGetCheck(lua_State *l, int index)
    {
        return (Vector *)LCheckUserdata(l, index, vector_meta_ptr, "vector");
    }

    inline Vector *LVectorGetNoCheck(lua_State *l, int index)
    {
        return (Vector *)LGetUserdata(l, index, vector_meta_ptr);
    }

    inline QAngle *LAngleGetCheck(lua_State *l, int index)
    {
        return (QAngle *)LCheckUserdata(l, index, vector_meta_ptr, "vector");
    }

    inline QAngle *LAngleGetNoCheck(lua_State *l, int index)
    {
        return (QAngle *)LGetUserdata(l, index, vector_meta_ptr);
    }

    int LVectorGet(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        int index = luaL_checkinteger(l, 2);

        luaL_argcheck(l, index >= 1 && index <= 3, 2, "index out of 1-3 range");

        lua_pushnumber(l, (*vec)[index - 1]);
        return 1;
    }

    int LVectorGetX(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        lua_pushnumber(l, vec->x);
        return 1;
    }

    int LVectorGetY(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        lua_pushnumber(l, vec->y);
        return 1;
    }

    int LVectorGetZ(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        lua_pushnumber(l, vec->z);
        return 1;
    }

    int LVectorSet(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        int index = luaL_checkinteger(l, 2);
        float value = luaL_checknumber(l, 3);

        luaL_argcheck(l, index >= 1 && index <= 3, 2, "index out of 1-3 range");

        (*vec)[index - 1] = value;
        return 0;
    }

    int LVectorSetX(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        float value = luaL_checknumber(l, 2);

        vec->x = value;
        return 0;
    }

    int LVectorSetY(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        float value = luaL_checknumber(l, 2);

        vec->x = value;
        return 0;
    }

    int LVectorSetZ(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        float value = luaL_checknumber(l, 2);

        vec->x = value;
        return 0;
    }

    int LVectorIndex(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        int index = -1;

        // Read index from offset
        if (lua_isnumber(l, 2)) {
            index = luaL_checkinteger(l, 2);
            luaL_argcheck(l, index >= 1 && index <= 3, 2, "index out of 1-3 range");
        }
        // Read index from x/y/z
        else {
            const char *str = luaL_checkstring(l, 2);
            if (str != nullptr && str[0] != '\0' && str[1] == '\0') {
                switch(str[0]) {
                    case 'x': index = 1; break;
                    case 'y': index = 2; break;
                    case 'z': index = 3; break;
                }
            }
        }
        if (index != -1) {
            lua_pushnumber(l, (*vec)[index - 1]);
        }
        // Get function
        else {
            lua_getmetatable(l, 1);
            lua_pushvalue(l, 2);
            lua_rawget(l, 3);
        }
        return 1;
    }

    int LVectorSetIndex(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        int index = -1;
        if (lua_isnumber(l, 2)) {
            index = luaL_checkinteger(l, 2);
            luaL_argcheck(l, index >= 1 && index <= 3, 2, "index out of 1-3 range");
        }
        else {
            const char *str = luaL_checkstring(l, 2);
            if (str != nullptr && str[0] != '\0' && str[1] == '\0') {
                switch(str[0]) {
                    case 'x': index = 1; break;
                    case 'y': index = 2; break;
                    case 'z': index = 3; break;
                }
            }
        }
        if (index != -1) {
            float value = luaL_checknumber(l, 3);

            (*vec)[index - 1] = value;
        }
        return 0;
    }

    int LVectorLength(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);

        lua_pushnumber(l, vec->Length());
        return 1;
    }

    int LVectorEquals(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *vec2 = LVectorGetCheck(l, 2);

        lua_pushboolean(l, vec == vec2);
        return 1;
    }

    int LVectorDistance(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *vec2 = LVectorGetCheck(l, 2);

        lua_pushnumber(l, vec->DistTo(*vec2));
        return 1;
    }

    int LVectorDot(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *vec2 = LVectorGetCheck(l, 2);

        lua_pushnumber(l, vec->Dot(*vec2));
        return 1;
    }

    int LVectorCross(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *vec2 = LVectorGetCheck(l, 2);

        Vector *out = LVectorAlloc(l);
        CrossProduct(*vec, *vec2, *out);
        return 1;
    }

    int LVectorRotate(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        QAngle *angle = LAngleGetCheck(l, 2);

        Vector *out = LVectorAlloc(l);
        VectorRotate(*vec, *angle, *out);
        return 1;
    }

    int LVectorToAngles(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);

        QAngle *out = LAngleAlloc(l);
        VectorAngles(*vec, *out);
        return 1;
    }
    
    int LVectorNormalize(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);

        Vector *out = LVectorAlloc(l);
        *out = *vec;
        VectorNormalize(*out);
        return 1;
    }
    
    int LVectorGetForward(lua_State *l)
    {
        QAngle *angle = LAngleGetCheck(l, 1);

        Vector *out1 = LVectorAlloc(l);
        AngleVectors(*angle, out1);
        return 1;
    }
    
    int LVectorGetAngleVectors(lua_State *l)
    {
        QAngle *angle = LAngleGetCheck(l, 1);

        Vector *out1 = LVectorAlloc(l);
        Vector *out2 = LVectorAlloc(l);
        Vector *out3 = LVectorAlloc(l);
        AngleVectors(*angle, out1, out2, out3);
        return 3;
    }
    
    int LVectorAdd(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *out = LVectorAlloc(l);
        if (lua_isnumber(l, 2)) {
            float num = lua_tonumber(l, 2);
            out->Init(vec->x+num,vec->y+num,vec->z+num);
        }
        else {
            Vector *vec2 = LVectorGetCheck(l, 2);
            *out = *vec + *vec2;
        }
        return 1;
    }

    int LVectorSubtract(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *out = LVectorAlloc(l);
        if (lua_isnumber(l, 2)) {
            float num = lua_tonumber(l, 2);
            out->Init(vec->x-num,vec->y-num,vec->z-num);
        }
        else {
            Vector *vec2 = LVectorGetCheck(l, 2);
            *out = *vec - *vec2;
        }
        return 1;
    }

    int LVectorMultiply(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *out = LVectorAlloc(l);
        if (lua_isnumber(l, 2)) {
            float num = lua_tonumber(l, 2);
            out->Init(vec->x*num,vec->y*num,vec->z*num);
        }
        else {
            Vector *vec2 = LVectorGetCheck(l, 2);
            *out = *vec * *vec2;
        }
        return 1;
    }

    int LVectorDivide(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *out = LVectorAlloc(l);
        if (lua_isnumber(l, 2)) {
            float num = lua_tonumber(l, 2);
            out->Init(vec->x/num,vec->y/num,vec->z/num);
        }
        else {
            Vector *vec2 = LVectorGetCheck(l, 2);
            *out = *vec / *vec2;
        }
        return 1;
    }

    int LVectorCopy(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        Vector *vec2 = LVectorGetCheck(l, 2);
        *vec = *vec2;
        return 0;
    }

    int LVectorCopyUnpacked(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);
        float x = luaL_checknumber(l, 2);
        float y = luaL_checknumber(l, 3);
        float z = luaL_checknumber(l, 4);

        LVectorAlloc(l)->Init(x, y, z);
        return 0;
    }

    int LVectorToString(lua_State *l)
    {
        Vector *vec = LVectorGetCheck(l, 1);

        lua_pushfstring(l, "%f %f %f", vec->x, vec->y, vec->z);
        return 1;
    }

    inline const EHANDLE *LEntityAlloc(lua_State *l, const EHANDLE oldhandle)
    {
        EHANDLE *handle = (EHANDLE *)lua_newuserdata(l, sizeof(EHANDLE));
        *handle = oldhandle;
        //luaL_getmetatable(l, "entity");
        lua_rawgeti(l, LUA_REGISTRYINDEX, entity_meta);
        lua_setmetatable(l, -2);
        return handle;
    }

    const EHANDLE invalid_handle;
    //!! Returns invalid handle ref if entity is null
    const EHANDLE *LEntityAlloc(lua_State *l, CBaseEntity *entity)
    {
        if (entity == nullptr) {
            lua_pushnil(l);
            return &invalid_handle;
        }
        EHANDLE *handle = (EHANDLE *)lua_newuserdata(l, sizeof(EHANDLE));
        handle->Set(entity);
        //luaL_getmetatable(l, "entity");
        lua_rawgeti(l, LUA_REGISTRYINDEX, entity_meta);
        lua_setmetatable(l, -2);
        return handle;
    }

    inline CBaseEntity *LEntityFindByIndex(int index)
    {
        if (index > 2048)
            return EHANDLE::FromIndex(index);
        else
            return UTIL_EntityByIndex(index);
    }

    int LEntityNew(lua_State *l)
    {
        if (lua_type(l, 1) == LUA_TNUMBER) {
            auto entity = LEntityFindByIndex(lua_tointeger(l, 1));
            auto handle = LEntityAlloc(l,entity);
        }
        else if (lua_isstring(l, 1)) {
            auto entity = CreateEntityByName(lua_tostring(l, 1));
            auto handle = LEntityAlloc(l,entity);
            if (entity != nullptr && (lua_gettop(l) < 2 || lua_toboolean(l,2)) ) {
                DispatchSpawn(entity);
            }
            if (entity != nullptr && (lua_gettop(l) < 3 || lua_toboolean(l,3)) ) {
                entity->Activate();
            }
        }
        else {
            luaL_error(l, "Expected classname string or entity index");
        }
        return 1;
    }    

    // Returns entity handle, throws an error if variable is not nil or entity
    inline const EHANDLE *LEntityGetCheck(lua_State *l, int index)
    {
        if (lua_type(l, index) == LUA_TNIL ) return &invalid_handle;

        return (const EHANDLE *)LCheckUserdata(l, index, entity_meta_ptr, "entity");
    }

    // Returns entity, or null if the variable is not a valid entity
    inline CBaseEntity *LEntityGetOptional(lua_State *l, int index)
    {
        auto *handle = (const EHANDLE *)LGetUserdata(l, index, entity_meta_ptr);

        return handle != nullptr ? handle->Get() : nullptr;
    }

    // Returns entity and throws an error if variable is not an entity or the entity is invalid
    inline CBaseEntity *LEntityGetNonNull(lua_State *l, int index)
    {
        auto ud = (const EHANDLE *)LCheckUserdata(l, index, entity_meta_ptr, "entity");
        auto entity = ud->Get();
        luaL_argcheck(l, entity != nullptr, index, "entity handle is invalid");
        return entity;
    }

    // Returns player entity and throws an error if variable is not an entity, the entity is invalid or not a player
    CTFPlayer *LPlayerGetNonNull(lua_State *l, int index)
    {
        auto ud = (const EHANDLE *)LCheckUserdata(l, index, entity_meta_ptr, "entity");
        auto entity = ud->Get();
        luaL_argcheck(l, entity != nullptr, index, "entity handle is invalid");
        auto player = ToTFPlayer(entity);
        luaL_argcheck(l, player != nullptr, index, "entity is not a player");
        return player;
    }


    int LEntityEquals(lua_State *l)
    {
        auto *handlel = LEntityGetCheck(l, 1);
        auto *handler = LEntityGetCheck(l, 2);

        lua_pushboolean(l, *handlel == *handler || handlel->Get() == handler->Get());
        return 1;
    }

    int LIsValid(lua_State *l)
    {
        lua_pushboolean(l, LEntityGetOptional(l, 1) != nullptr);
        return 1;
    }

    int LEntityIsValid(lua_State *l)
    {
        auto *handle = LEntityGetCheck(l, 1);

        lua_pushboolean(l, handle->Get() != nullptr);
        return 1;
    }

    int LEntityGetHandleIndex(lua_State *l)
    {
        auto *handle = LEntityGetCheck(l, 1);

        lua_pushinteger(l, handle->ToInt());
        return 1;
    }

    int LEntityGetNetworkIndex(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
    
        lua_pushinteger(l, entity->edict() != nullptr ? entity->entindex() : -1);
        return 1;
    }

    int LEntitySpawn(lua_State *l)
    {
        servertools->DispatchSpawn(LEntityGetNonNull(l, 1));
        return 0;
    }

    int LEntityActivate(lua_State *l)
    {
        LEntityGetNonNull(l, 1)->Activate();
        return 0;
    }

    int LEntityRemove(lua_State *l)
    {
        LEntityGetNonNull(l, 1)->Remove();
        return 0;
    }

    int LEntityGetName(lua_State *l)
    {
        lua_pushstring(l, STRING(LEntityGetNonNull(l, 1)->GetEntityName()));
        return 1;
    }

    int LEntitySetName(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        char nameLower[1024];
        StrLowerCopy(name, nameLower);

        entity->SetName(AllocPooledString(nameLower));
        return 0;
    }

    int LEntityGetPlayerName(lua_State *l)
    {
        auto entity = LPlayerGetNonNull(l, 1);
        lua_pushstring(l, entity->GetPlayerName());
        return 1;
    }

    int LEntityIsAlive(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->IsAlive());
        return 1;
    }

    int LEntityIsPlayer(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->IsPlayer());
        return 1;
    }

    int LEntityIsNPC(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && !entity->IsPlayer() && entity->MyNextBotPointer() != nullptr);
        return 1;
    }

    int LEntityIsBot(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, ToTFBot(entity) != nullptr);
        return 1;
    }

    int LEntityIsRealPlayer(lua_State *l)
    {
        auto entity = ToTFPlayer(LEntityGetOptional(l, 1));
        lua_pushboolean(l, entity != nullptr && entity->IsRealPlayer());
        return 1;
    }

    int LEntityIsWeapon(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->IsBaseCombatWeapon());
        return 1;
    }

    int LEntityIsObject(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->IsBaseObject());
        return 1;
    }

    int LEntityIsCombatCharacter(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->MyCombatCharacterPointer() != nullptr);
        return 1;
    }

    int LEntityIsWearable(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity != nullptr && entity->IsWearable());
        return 1;
    }

    int LEntityIsWorld(lua_State *l)
    {
        auto entity = LEntityGetOptional(l, 1);
        lua_pushboolean(l, entity == GetWorldEntity());
        return 1;
    }

    int LEntityGetClassname(lua_State *l)
    {
        auto *handle = LEntityGetCheck(l, 1);
        auto entity = handle->Get();
        lua_pushstring(l, entity->GetClassname());
        return 1;
    }

    enum CallbackType
    {
        ON_REMOVE,
        ON_SPAWN,
        ON_ACTIVATE,
        ON_DAMAGE_RECEIVED_PRE,
        ON_DAMAGE_RECEIVED_POST,
        ON_INPUT,
        ON_OUTPUT,
        ON_KEY_PRESSED,
        ON_KEY_RELEASED,
        ON_DEATH,
        ON_EQUIP_ITEM,
        ON_DEPLOY_WEAPON,
        ON_PROVIDE_ITEMS,
        ON_TOUCH,
        ON_START_TOUCH,
        ON_END_TOUCH,
        ON_SHOULD_COLLIDE,
        CALLBACK_TYPE_COUNT
    };

    class EntityCallback 
    {
    public:
        EntityCallback(LuaState *state, int func, int id) : state(state), func(func), id(id) {};

        ~EntityCallback() {
            if (state != nullptr) {
                luaL_unref(state->GetState(), LUA_REGISTRYINDEX, func);
            }
        };
        LuaState *state;
        int func;
        int id;
        bool deleted = false;
        int calling = false;
    };

    struct EntityTableStorageEntry
    {
        LuaState *state;
        int value;
    };

    class LuaEntityModule : public EntityModule
    {
    public:
        LuaEntityModule(CBaseEntity *entity) : EntityModule(entity), owner(entity) {};

        virtual ~LuaEntityModule() {
            if (owner != nullptr && owner->IsMarkedForDeletion()) {
                FireCallback(ON_REMOVE);
            }
            for (auto state : states) {
                state->EntityDeleted(owner);
            }
            //for (auto &entry : tableStore) {
            //    luaL_unref(entry.state->GetState(), LUA_REGISTRYINDEX, entry.value);
            //}
        }

        // LuaState removed, remove associated callbacks
        void LuaStateRemoved(LuaState *state) {
            for (int callbackType = 0; callbackType < CALLBACK_TYPE_COUNT; callbackType++) {
                RemoveIf(callbacks[callbackType], [&](auto &pair) {
                    if (pair.state == state) {

                        // Set to null to prevent destructor trying to unref function
                        pair.state = nullptr;
                        return true;
                    }
                    return false;
                });
            }
            states.erase(state);
            //std::remove_if(tableStore.begin(), tableStore.end(), [&](auto &entry) {
            //    return entry.state == state;
            //});
        }

        int AddCallback(CallbackType type, LuaState *state, int func) {
            if (type < 0 && type >= CALLBACK_TYPE_COUNT) return -1;

            callbacks[type].emplace_back(state, func, callbackNextId);
            states.insert(state);
            state->EntityCallbackAdded(owner);
            return callbackNextId++;
        }

        void RemoveCallback(CallbackType type, LuaState *state, int id) {
            for (auto it = callbacks[type].begin(); it != callbacks[type].end(); it++) {
                auto &pair = *it;
                if (id == pair.id && pair.state == state) {
                    pair.deleted = true;
                    if (pair.calling == 0) {
                        callbacks[type].erase(it);
                    }
                    return;
                }
            }
        }

        void RemoveCallback(LuaState *state, int id) {
            for (int type = 0; type < CALLBACK_TYPE_COUNT; type++) {
                RemoveCallback((CallbackType)type, state, id);
            }
        }

        // bool RemoveTableStorageEntry(LuaState *state, int value) {
        //     if (tableStore.empty()) return false;

        //     if (RemoveFirstElement(tableStore, [&](auto &entry){
        //         return entry.state == state && entry.value == value;
        //     })) {
        //         luaL_unref(state->GetState(), LUA_REGISTRYINDEX, value);
        //     }

        //     return false;
        // }

        // void SetTableStorageEntry(LuaState *state, int value) {
        //     for(auto &entry : tableStore) {
        //         if (entry.state == state && entry.value == value) {
        //             return;
        //         }
        //     }
        //     tableStore.push_back({state, value});
        // }

        void CallCallback(std::list<EntityCallback> callbackList, std::list<EntityCallback>::iterator &it, int args, int ret) {
            auto &callback = *it;
            callback.calling++;
            callback.state->Call(args, ret);
            callback.calling--;
            if (callback.deleted && callback.calling == 0) {
                it = callbackList.erase(it);
            } else {
                it++;
            }
        }

        void FireCallback(CallbackType type, std::function<void(LuaState *)> *extrafunc = nullptr, int extraargs = 0, std::function<void(LuaState *)> *extraretfunc = nullptr, int ret = 0) {
            auto &callbackList = callbacks[type];
            if (callbackList.empty()) return;
            
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto state = callback.state;
                auto l = state->GetState();
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, owner);
                if (extrafunc != nullptr)
                    (*extrafunc)(state);
                CallCallback(callbackList, it, 1 + extraargs, ret);
                if (extraretfunc != nullptr)
                    (*extraretfunc)(state);

                lua_settop(l, 0);
            }
        }

        std::list<EntityCallback> callbacks[CALLBACK_TYPE_COUNT];
        std::unordered_set<LuaState *> states;
        CBaseEntity *owner;
        int inCallback = 0;
        int callbackNextId = 0;
        CTakeDamageInfo lastTakeDamage;
        //std::vector<EntityTableStorageEntry> tableStore;
    };

    int LEntityAddCallback(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        int type = luaL_checkinteger(l, 2);
        luaL_argcheck(l, type >= 0 && type < CALLBACK_TYPE_COUNT, 2, "type out of range");
        luaL_checktype(l, 3, LUA_TFUNCTION);
        int func = luaL_ref(l, LUA_REGISTRYINDEX);

        int id = entity->GetOrCreateEntityModule<LuaEntityModule>("luaentity")->AddCallback((CallbackType)type, cur_state, func);
        lua_pushinteger(l, id);
        return 1;
    }

    int LEntityRemoveCallback(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        int type = lua_gettop(l) > 2 ? luaL_checkinteger(l, 2) : -1;
        luaL_argcheck(l, type >= -1 && type < CALLBACK_TYPE_COUNT, 2, "type out of range");
        int id = luaL_checkinteger(l, type != -1 ? 3 : 2);

        auto module = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (module != nullptr) {
            if (type != -1)
                module->RemoveCallback((CallbackType)type, cur_state, id);
            else
                module->RemoveCallback(cur_state, id);
        }
        return 0;
    }

    void LFromVariant(lua_State *l, variant_t &variant)
    {
        auto fieldType = variant.FieldType();
        switch (fieldType) {
            case FIELD_VOID: lua_pushnil(l); return;
            case FIELD_BOOLEAN: lua_pushboolean(l,variant.Bool()); return;
            case FIELD_CHARACTER: case FIELD_SHORT: case FIELD_INTEGER: variant.Convert(FIELD_INTEGER); lua_pushinteger(l,variant.Int()); return;
            case FIELD_FLOAT: lua_pushnumber(l,variant.Float()); return;
            case FIELD_STRING: lua_pushstring(l,variant.String()); return;
            /*case FIELD_CUSTOM: {
                int ref;
                variant.SetOther(&ref);
                lua_rawgeti(l, LUA_REGISTRYINDEX, ref);
                return;
            }*/
            case FIELD_POSITION_VECTOR: case FIELD_VECTOR: {
                variant.Convert(FIELD_VECTOR);
                auto vec = LVectorAlloc(l);
                variant.Vector3D(*vec);
                return;
            }
            case FIELD_EHANDLE: case FIELD_CLASSPTR: variant.Convert(FIELD_EHANDLE); LEntityAlloc(l, variant.Entity()); return;
            default: lua_pushnil(l); return;
        }
        
    }

    void LToVariant(lua_State *l, int index, variant_t &variant)
    {
        int type = lua_type(l, index);
        if (type == LUA_TNIL) {

        }
        else if (type == LUA_TBOOLEAN) {
            variant.SetBool(lua_toboolean(l, index));
        }
        else if (type == LUA_TNUMBER) {
            double number = lua_tonumber(l, index);
            int inumber = (int) number;
            if (number == inumber) {
                variant.SetInt(inumber);
            }
            else {
                variant.SetFloat(number);
            }
        }
        else if (type != LUA_TUSERDATA && type != LUA_TLIGHTUSERDATA) {
            variant.SetString(AllocPooledString(lua_tostring(l,index)));
        }
        else if (luaL_testudata(l, index, "vector")) {
            variant.SetVector3D(*(Vector*)lua_touserdata(l, index));
        }
        else if (luaL_testudata(l, index, "entity")) {
            variant.SetEntity(*(EHANDLE*)lua_touserdata(l, index));
        }
        /*else if (allowTables) {
            lua_pushvalue(l, index);
            int ref = luaL_ref(l, LUA_REGISTRYINDEX);
            variant.Set(FIELD_CUSTOM, &ref);
        }*/
    }

    int LEntityAcceptInput(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        const char *name = luaL_checkstring(l, 2);
        variant_t variant;
        if (lua_gettop(l) > 2)
            LToVariant(l, 3, variant);

        CBaseEntity *activator = lua_gettop(l) > 3 ? LEntityGetCheck(l, 4)->Get() : nullptr;
        CBaseEntity *caller = lua_gettop(l) > 4 ? LEntityGetCheck(l, 5)->Get() : nullptr;
        
        lua_pushboolean(l, entity->AcceptInput(name, activator, caller, variant, -1));
        return 1;
    }

    int LEntityGetPlayerItemBySlot(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        int slot = luaL_checkinteger(l, 2);
        if (player != nullptr) {
            LEntityAlloc(l, GetEconEntityAtLoadoutSlot(player, slot));
            return 1;
        }
        lua_pushnil(l);
        return 1;
    }

    int LEntityGetPlayerItemByName(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        const char *name = luaL_checkstring(l, 2);
        if (player != nullptr) {
            CBaseEntity *ret = nullptr;
            ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), name)) {
                    ret = entity;
                    return false;
                }
                return true;
            });
            LEntityAlloc(l, ret);
            return 1;
        }
        lua_pushnil(l);
        return 1;
    }

    int LEntityGetItemAttribute(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);

        if (entity == nullptr) {
            luaL_error(l, "Entity in not valid");
            lua_pushnil(l);
            return 1;
        }

        CAttributeList *list = nullptr;
        if (entity->IsPlayer()) {
            list = ToTFPlayer(entity)->GetAttributeList();
        } 
        else {
            CEconEntity *econEntity = rtti_cast<CEconEntity *>(entity);
            if (econEntity != nullptr) {
                list = &econEntity->GetItem()->GetAttributeList();
            }
        }
        if (list == nullptr) {
            luaL_error(l, "Entity (%s) is not a player or item", entity->GetClassname());
            lua_pushnil(l);
            return 1;
        }
        CEconItemAttribute * attr = list->GetAttributeByName(name);
        if (attr != nullptr) {
            if (!attr->GetStaticData()->IsType<CSchemaAttributeType_String>()) {
                lua_pushnumber(l,attr->GetValuePtr()->m_Float);
            }
            else {
                char buf[256];
                attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
                lua_pushstring(l,buf);
            }
        }
        else {
            lua_pushnil(l);
        }
        return 1;
    }

    int LEntitySetItemAttribute(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        auto value = lua_tostring(l, 3);

        CAttributeList *list = nullptr;
        if (entity->IsPlayer()) {
            list = ToTFPlayer(entity)->GetAttributeList();
        } 
        else {
            CEconEntity *econEntity = rtti_cast<CEconEntity *>(entity);
            if (econEntity != nullptr) {
                list = &econEntity->GetItem()->GetAttributeList();
            }
        }
        if (list == nullptr) {
            luaL_error(l, "Entity (%s) is not a player or item", entity->GetClassname());
            return 0;
        }
        auto def = GetItemSchema()->GetAttributeDefinitionByName(name);
        if (def != nullptr) {
            if (value == nullptr) {
                list->RemoveAttribute(def);
            }
            else {
                list->AddStringAttribute(def, value);
            }
        }
        else {
            luaL_error(l, "Invalid attribute name: %s", name);
        }
        return 0;
    }

    int LEntityToString(lua_State *l)
    {
        auto *handle = LEntityGetCheck(l, 1);
        auto entity = handle->Get();

        if (entity != nullptr) {
            lua_pushfstring(l, "Entity handle id: %d , classname: %s , targetname: %s , net id: %d", handle->ToInt(), entity->GetClassname(), STRING(entity->GetEntityName()), entity->entindex());
        }
        else {
            lua_pushfstring(l, "Invalid entity handle id: %d", handle->ToInt());
        }
        
        return 1;
    }

    void FindSendProp(int& off, SendTable *s_table, std::function<void(SendProp *, int)> onfound)
    {
        for (int i = 0; i < s_table->GetNumProps(); ++i) {
            SendProp *s_prop = s_table->GetProp(i);
            //Msg("Prop %d %s %d %d %d\n", s_prop, s_prop->GetName(), s_prop->GetDataTable(), s_prop->GetDataTable() != nullptr && s_prop->GetDataTable()->GetNumProps() > 1, s_prop->GetDataTable() != nullptr && s_prop->GetDataTable()->GetNumProps() > 1 && strcmp(s_prop->GetDataTable()->GetProp(0)->GetName(), "000") == 0);
            if (s_prop->IsInsideArray())
                continue;

            if (s_prop->GetDataTable() == nullptr || (s_prop->GetDataTable() != nullptr && s_prop->GetDataTable()->GetNumProps() > 1 && strcmp(s_prop->GetDataTable()->GetProp(0)->GetName(), "000") == 0)) {
                onfound(s_prop, off + s_prop->GetOffset());
            }
            else if (s_prop->GetDataTable() != nullptr) {
                off += s_prop->GetOffset();
                FindSendProp(off, s_prop->GetDataTable(), onfound);
                off -= s_prop->GetOffset();
            }
        }
    }

    void *stringSendProxy = nullptr;
    CStandardSendProxies* sendproxies = nullptr;
    int LEntityDumpProperties(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);

        int off = 0;
        
        lua_newtable(l);
        FindSendProp(off, entity->GetServerClass()->m_pTable, [&](SendProp *prop, int offset){
            Mod::Etc::Mapentity_Additions::PropCacheEntry entry;
            Mod::Etc::Mapentity_Additions::GetSendPropInfo(prop, entry, offset);
            if (entry.arraySize > 1) {
                lua_newtable(l);
            }
            for (int i = 0; i < entry.arraySize; i++) {
                variant_t variant;
                Mod::Etc::Mapentity_Additions::ReadProp(entity, entry, variant, i, -1);
                //Msg("Sendprop %s = %s %d %d\n", prop->GetName(), variant.String(), entry.offset, entry.arraySize);

                LFromVariant(l, variant);
                if (entry.arraySize == 1) {
                    lua_setfield(l, -2, prop->GetName());
                }
                else {
                    lua_rawseti(l, -2, i + 1);
                }
            }
            if (entry.arraySize > 1) {
                lua_setfield(l, -2, prop->GetName());
            }
        });

        for (datamap_t *dmap = entity->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].fieldName != nullptr) {
                    Mod::Etc::Mapentity_Additions::PropCacheEntry entry;
                    Mod::Etc::Mapentity_Additions::GetDataMapInfo(dmap->dataDesc[i], entry);
                    if (entry.arraySize > 1) {
                        lua_newtable(l);
                    }
                    for (int j = 0; j < entry.arraySize; j++) {
                        variant_t variant;
                        Mod::Etc::Mapentity_Additions::ReadProp(entity, entry, variant, j, -1);
                        //Msg("Datamap %s = %s %d\n", dmap->dataDesc[i].fieldName, variant.String(), entry.offset);

                        LFromVariant(l, variant);
                        if (entry.arraySize == 1) {
                            lua_setfield(l, -2, dmap->dataDesc[i].fieldName);
                        }
                        else {
                            lua_rawseti(l, -2, j + 1);
                        }
                    }
                    if (entry.arraySize > 1) {
                        lua_setfield(l, -2, dmap->dataDesc[i].fieldName);
                    }
                }
            }
        }

        auto vars = GetCustomVariables(entity);
        for (auto var : vars) {
            LFromVariant(l, var.value);
            lua_setfield(l, -2, STRING(var.key));
        }

        return 1;
    }

    
    int LEntityDumpInputs(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        lua_newtable(l);
        int j = 0;
        for (datamap_t *dmap = entity->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].externalName != nullptr && (dmap->dataDesc[i].flags & FTYPEDESC_INPUT)) {
                    lua_pushstring(l, dmap->dataDesc[i].externalName);
                    lua_rawseti(l, -2, ++j);
                }
            }
        }
        for (auto &filter : Mod::Etc::Mapentity_Additions::InputFilter::List()) {
            if (filter->Test(entity)) {
                for (auto &input : filter->inputs) {
                    lua_pushfstring(l, "$%d%s",input.name, input.prefix ? "<var>" : "");
                    lua_rawseti(l, -2, ++j);
                    //if ((!input.prefix && CompareCaseInsensitiveStringView(inputNameLower, input.name)) ||
                    //    (input.prefix && CompareCaseInsensitiveStringViewBeginsWith(inputNameLower, input.name))) {
                    //    return &input.func;
                    //}
                }
            }
        }
        return 1;
    }
    struct InputCacheEntry {
        inputfunc_t inputptr = nullptr;
        Mod::Etc::Mapentity_Additions::CustomInputFunction *custominputptr = nullptr;
    };

    /*bool CheckInput(CBaseEntity *entity) {

        auto datamap = entity->GetDataDescMap();
        for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].fieldName != nullptr && strcmp(dmap->dataDesc[i].fieldName, nameNoArray.c_str()) == 0) {
                    fieldtype_t fieldType = isVecAxis ? FIELD_FLOAT : dmap->dataDesc[i].fieldType;
                    int offset = dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] + extraOffset;
                    
                    offset += clamp(arrayPos, 0, (int)dmap->dataDesc[i].fieldSize) * (dmap->dataDesc[i].fieldSizeInBytes / dmap->dataDesc[i].fieldSize);

                    datamap_cache.push_back({datamap, name, offset, fieldType, dmap->dataDesc[i].fieldSize});
                    return datamap_cache.back();
                }
            }
        }
    }*/
    std::vector<datamap_t *> input_cache_classes;
    std::vector<std::pair<std::vector<std::string>, std::vector<InputCacheEntry>>> input_cache;

    struct ArrayProp
    {
        CBaseEntity *entity;
        Mod::Etc::Mapentity_Additions::PropCacheEntry entry;
    };

    ArrayProp *LPropAlloc(lua_State *l, CBaseEntity *entity, Mod::Etc::Mapentity_Additions::PropCacheEntry &entry)
    {
        ArrayProp *prop = (ArrayProp *)lua_newuserdata(l, sizeof(ArrayProp));
        prop->entity = entity;
        prop->entry = entry;
        //luaL_getmetatable(l, "entity");
        lua_rawgeti(l, LUA_REGISTRYINDEX, prop_meta);
        lua_setmetatable(l, -2);
        return prop;
    }

    int LPropGet(lua_State *l)
    {
        auto prop = (ArrayProp *) lua_touserdata(l, 1);
        int index = lua_tointeger(l, 2);
        luaL_argcheck(l, prop != nullptr, 1, "Userdata is null");
        luaL_argcheck(l, index > 0 && index <= prop->entry.arraySize, 2, "index out of range");

        variant_t variant;
        Mod::Etc::Mapentity_Additions::ReadProp(prop->entity, prop->entry, variant, index - 1, -1);
        LFromVariant(l, variant);
        return 1;
    }

    int LPropGetN(lua_State *l)
    {
        auto prop = (ArrayProp *) lua_touserdata(l, 1);
        luaL_argcheck(l, prop != nullptr, 1, "Userdata is null");

        lua_pushinteger(l, prop->entry.arraySize);
        return 1;
    }

    int LPropSet(lua_State *l)
    {
        auto prop = (ArrayProp *) lua_touserdata(l, 1);
        int index = lua_tointeger(l, 2);
        luaL_argcheck(l, prop != nullptr, 1, "Userdata is null");
        luaL_argcheck(l, index > 0 && index <= prop->entry.arraySize, 2, "index out of range");

        variant_t variant;
        LToVariant(l, 3, variant);
        Mod::Etc::Mapentity_Additions::WriteProp(prop->entity, prop->entry, variant, index, -1);
        return 0;
    }

    inline InputCacheEntry &LGetInputCacheEntry(CBaseEntity *entity, std::string &name)
    {
        auto datamap = entity->GetDataDescMap();
        size_t classIndex = 0;
        for (; classIndex < input_cache_classes.size(); classIndex++) {
            if (input_cache_classes[classIndex] == datamap) {
                break;
            }
        }
        if (classIndex >= input_cache_classes.size()) {
            input_cache_classes.push_back(datamap);
            input_cache.emplace_back();
        }
        auto &pair = input_cache[classIndex];
        auto &names = pair.first;

        size_t nameCount = names.size();
        for (size_t i = 0; i < nameCount; i++ ) {
            if (names[i] == name) {
                return pair.second[i];
            }
        }
        for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].fieldName != nullptr && (dmap->dataDesc[i].flags & FTYPEDESC_INPUT) && (strcmp(dmap->dataDesc[i].externalName, name.c_str()) == 0)) {
                    names.push_back(name);
                    pair.second.push_back({dmap->dataDesc[i].inputFunc, nullptr});
                    return pair.second.back();
                }
            }
        }
        auto func = Mod::Etc::Mapentity_Additions::GetCustomInput(entity, name[0] == '$' ? name.c_str() + 1 : name.c_str());
        if (func != nullptr) {
            names.push_back(name);
            pair.second.push_back({nullptr, func});
            return pair.second.back();
        }
        
        names.push_back(name);
        pair.second.push_back({nullptr, nullptr});
        return pair.second.back();
    }

    int LEntityCallInputNormal(lua_State *l)
    {
        auto func = MakePtrToMemberFunc<CBaseEntity, void, inputdata_t &>((void *)(uintptr_t) lua_tointeger(l, lua_upvalueindex(1)));
        auto entity = LEntityGetNonNull(l, 1);
        inputdata_t data;
        LToVariant(l, 2, data.value);
        data.pActivator = LEntityGetOptional(l, 3);
        data.pCaller = LEntityGetOptional(l, 4);
        data.nOutputID = -1;
        (entity->*func)(data);

        return 0;
    }

    int LEntityCallInputCustom(lua_State *l)
    {
        auto func = (Mod::Etc::Mapentity_Additions::CustomInputFunction *)(uintptr_t) lua_tointeger(l, lua_upvalueindex(1));
        auto entity = LEntityGetNonNull(l, 1);
        variant_t value;
        LToVariant(l, 2, value);
        (*func)(entity, lua_tostring(l, lua_upvalueindex(2)), LEntityGetOptional(l,3), LEntityGetOptional(l,4), value);
        return 0;
    }

    int LEntityGetProp(lua_State *l)
    {
        // Get function
        lua_getmetatable(l, 1);
        lua_pushvalue(l, 2);
        lua_rawget(l, 3);
        if (!lua_isnil(l,4)) return 1;

        auto entity = LEntityGetNonNull(l, 1);
        
        // Table access first
        lua_rawgeti(l, LUA_REGISTRYINDEX, entity_table_store);
        if (lua_rawgetp(l, -1, entity) != LUA_TNIL) {
            lua_pushvalue(l, 2);
            if (lua_rawget(l, -2) != LUA_TNIL) {
                return 1;
            }
        }

        const char *varName = luaL_checkstring(l, 2);
        
        if (varName == nullptr) {
            lua_pushnil(l);
            return 1;
        }

        std::string varNameStr(varName);
        
        variant_t variant;
        
        if (entity->GetCustomVariableByText(varName, variant)) {
            LFromVariant(l, variant);
            return 1;
        }
        auto *entry = &Mod::Etc::Mapentity_Additions::GetDataMapOffset(entity->GetDataDescMap(), varNameStr);
        if (entry == nullptr || entry->offset <= 0) {
            entry = &Mod::Etc::Mapentity_Additions::GetSendPropOffset(entity->GetServerClass(), varNameStr);
        }
        if (entry != nullptr && entry->offset > 0) {
            if (entry->arraySize > 1 && entry->fieldType != FIELD_CHARACTER) {
                LPropAlloc(l, entity, *entry);
            }
            else{
                Mod::Etc::Mapentity_Additions::ReadProp(entity, *entry, variant, 0, -1);
                LFromVariant(l, variant);
            }
            return 1;
        }

        auto &input = LGetInputCacheEntry(entity, varNameStr);
        if (input.inputptr != nullptr) {
            lua_pushinteger(l, (uintptr_t) GetAddrOfMemberFunc(input.inputptr));
            lua_pushcclosure(l, LEntityCallInputNormal,1);
            return 1;
        } 
        else if (input.custominputptr != nullptr) {
            lua_pushinteger(l, (uintptr_t) input.custominputptr);
            lua_pushstring(l, varNameStr.c_str());
            lua_pushcclosure(l, LEntityCallInputCustom,2);
            return 1;
        } 
        lua_pushnil(l);
        return 1;
    }

    int LEntitySetProp(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);

        // Tables must be saved in special way
        if (lua_type(l, 3) == LUA_TTABLE) {
            lua_rawgeti(l, LUA_REGISTRYINDEX, entity_table_store);
            if (lua_rawgetp(l, -1, entity) == LUA_TNIL) {
                auto mod = entity->GetOrCreateEntityModule<LuaEntityModule>("luaentity");
                lua_newtable(l);
                lua_pushvalue(l,2);
                lua_pushvalue(l,3);
                lua_rawset(l, -3);

                lua_rawsetp(l, -3, entity);
                return 0;
            }
            lua_pushvalue(l,2);
            lua_pushvalue(l,3);
            lua_rawset(l, -3);
            return 0;
        }
        else {
            lua_rawgeti(l, LUA_REGISTRYINDEX, entity_table_store);
            if (lua_rawgetp(l, -1, entity) != LUA_TNIL) {
                lua_pushvalue(l,2);
                lua_pushnil(l);
                lua_rawset(l, -3);
            }
        }

        const char *varName = luaL_checkstring(l, 2);

        if (varName == nullptr) {
            lua_pushnil(l);
            return 1;
        }
       
        std::string varNameStr(varName);
        /*auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            variant_t variantPre;
            entity->GetCustomVariableByText(varName, variantPre);
            if (variantPre.FieldType() == FIELD_CUSTOM) {
                int ref;
                variantPre.SetOther(&ref);

            }
        }*/

        variant_t variant;
        LToVariant(l, 3, variant);
        SetEntityVariable(entity, Mod::Etc::Mapentity_Additions::ANY, varNameStr, variant, 0, -1);
        
        /*if (variant.FieldType() == FIELD_CUSTOM) {
            if (mod == nullptr) {
                entity->AddEntityModule("luaentity", mod = new LuaEntityModule(entity));
            }
            int ref;
            variant.SetOther(&ref);
            mod->SetTableStorageEntry(cur_state, ref);
        }*/

        return 0;
    }

    struct EntityCreateCallback
    {   
        LuaState *state;
        string_t classname;
        bool wildcard = false;
        int func;
    };
    std::vector<EntityCreateCallback> entity_create_callbacks;

    int LEntityAddCreateCallback(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        luaL_argcheck(l, strlen(name) > 0, 1, "entity classname empty");
        luaL_checktype(l,2, LUA_TFUNCTION);
        int func = luaL_ref(l, LUA_REGISTRYINDEX);
        
        entity_create_callbacks.push_back({cur_state, AllocPooledString(name), name[strlen(name) - 1] == '*', func});
        lua_pushinteger(l, func);
        return 1;
    }

    int LEntityRemoveCreateCallback(lua_State *l)
    {
        int func = luaL_checkinteger(l, 1);
        RemoveIf(entity_create_callbacks, [&](auto &callback){ 
            if (callback.state == cur_state && callback.func == func) {
                luaL_unref(l, LUA_REGISTRYINDEX, func);
                return true ;
            }
            return false ;
        });

        return 0;
    }

    void DamageInfoToTable(lua_State *l, const CTakeDamageInfo &info) {
        lua_newtable(l);
        LEntityAlloc(l, info.GetAttacker());
        lua_setfield(l, -2, "Attacker");
        LEntityAlloc(l, info.GetInflictor());
        lua_setfield(l, -2, "Inflictor");
        LEntityAlloc(l, info.GetWeapon());
        lua_setfield(l, -2, "Weapon");
        lua_pushnumber(l, info.GetDamage());
        lua_setfield(l, -2, "Damage");
        lua_pushinteger(l, info.GetDamageType());
        lua_setfield(l, -2, "DamageType");
        lua_pushinteger(l, info.GetDamageCustom());
        lua_setfield(l, -2, "DamageCustom");
        lua_pushinteger(l, info.GetCritType());
        lua_setfield(l, -2, "CritType");
        auto vec1 = LVectorAlloc(l);
        *vec1 = info.GetDamagePosition();
        lua_setfield(l, -2, "DamagePosition");
        vec1 = LVectorAlloc(l);
        *vec1 = info.GetDamageForce();
        lua_setfield(l, -2, "DamageForce");
        vec1 = LVectorAlloc(l);
        *vec1 = info.GetReportedPosition();
        lua_setfield(l, -2, "ReportedPosition");
    };

    void TableToDamageInfo(lua_State *l, int idx, CTakeDamageInfo &info) {
        lua_pushvalue(l, idx);
        lua_getfield(l, -1, "Attacker");
        info.SetAttacker(LEntityGetOptional(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "Inflictor");
        info.SetInflictor(LEntityGetOptional(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "Weapon");
        info.SetWeapon(LEntityGetOptional(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "Damage");
        info.SetDamage(lua_tonumber(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "DamageType");
        info.SetDamageType(lua_tointeger(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "DamageCustom");
        info.SetDamageCustom(lua_tointeger(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "CritType");
        info.SetCritType((CTakeDamageInfo::ECritType)lua_tointeger(l, -1));
        lua_pop(l, 1);

        lua_getfield(l, -1, "DamagePosition");
        auto vec = LVectorGetNoCheck(l, -1);
        if (vec != nullptr) 
            info.SetDamagePosition(*vec);
        lua_pop(l, 1);

        lua_getfield(l, -1, "DamageForce");
        vec = LVectorGetNoCheck(l, -1);
        if (vec != nullptr) 
            info.SetDamageForce(*vec);
        lua_pop(l, 1);

        lua_getfield(l, -1, "ReportedPosition");
        vec = LVectorGetNoCheck(l, -1);
        if (vec != nullptr) 
            info.SetReportedPosition(*vec);
        lua_pop(l, 1);
    };

    int LEntityTakeDamage(lua_State *l)
    {
        CTakeDamageInfo info;
        auto entity = LEntityGetNonNull(l, 1);
        luaL_checktype(l, 2, LUA_TTABLE);
        TableToDamageInfo(l, 2, info);
        lua_pushinteger(l, entity->TakeDamage(info));
        return 1;
    }

    int LEntityTakeHealth(lua_State *l)
    {
        CTakeDamageInfo info;
        auto entity = LEntityGetNonNull(l, 1);
        float amount = luaL_checknumber(l, 2);
        bool overheal = lua_toboolean(l, 3);
        entity->TakeHealth(amount, overheal ? DMG_IGNORE_MAXHEALTH : DMG_GENERIC);
        return 1;
    }

    int LEntityAddCond(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        int condition = luaL_checkinteger(l, 2);
        float duration = luaL_optnumber(l, 3, -1);
        auto provider = lua_gettop(l) > 3 ? ToTFPlayer(LEntityGetOptional(l, 4)) : nullptr;
        player->m_Shared->AddCond((ETFCond)condition, duration, provider);
        return 0;
    }

    int LEntityRemoveCond(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        int condition = luaL_checkinteger(l, 2);
        player->m_Shared->RemoveCond((ETFCond)condition);
        return 0;
    }

    int LEntityGetCondProvider(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        int condition = luaL_checkinteger(l, 2);
        LEntityAlloc(l, player->m_Shared->GetConditionProvider((ETFCond)condition));
        return 1;
    }

    int LEntityInCond(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        int condition = luaL_checkinteger(l, 2);
        lua_pushinteger(l, player->m_Shared->InCond((ETFCond)condition));
        return 1;
    }

    int LEntityStunPlayer(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        float duration = luaL_checknumber(l, 2);
        float amount = luaL_checknumber(l, 3);
        int flags = luaL_optnumber(l, 4, 1);
        auto stunner = lua_gettop(l) > 4 ? ToTFPlayer(LEntityGetOptional(l, 5)) : nullptr;
        player->m_Shared->StunPlayer(duration, amount, flags, stunner);
        return 0;
    }

    int LEntityGetAbsOrigin(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto *vec = LVectorAlloc(l);
        *vec = entity->GetAbsOrigin();
        return 1;
    }

    int LEntitySetAbsOrigin(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto vec = LVectorGetCheck(l, 2);
        entity->SetAbsOrigin(*vec);
        return 0;
    }

    int LEntityGetAbsAngles(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto *vec = LAngleAlloc(l);
        *vec = entity->GetAbsAngles();
        return 1;
    }

    int LEntitySetAbsAngles(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto vec = LAngleGetCheck(l, 2);
        entity->SetAbsAngles(*vec);
        return 0;
    }

    int LEntityTeleport(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto pos = LVectorGetNoCheck(l, 2);
        auto ang = LAngleGetNoCheck(l, 3);
        auto vel = LVectorGetNoCheck(l, 4);
        entity->Teleport(pos, ang, vel);
        return 0;
    }

    int LEntityCreateItem(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        auto entity = CreateItemByName(player, name);
        auto noRemove = lua_toboolean(l, 4);
        auto forceGive = lua_gettop(l) < 5 || lua_toboolean(l, 5);
        if (entity != nullptr && lua_gettop(l) > 2 && lua_type(l, 3) == LUA_TTABLE) {
            auto view = entity->GetItem();
            lua_pushnil(l);
            while (lua_next(l, 3) != 0) {
                if (lua_type(l, -2) == LUA_TSTRING) {
                    auto attrdef = GetItemSchema()->GetAttributeDefinitionByName(lua_tostring(l, -2));
                    if (attrdef != nullptr) {
                        view->GetAttributeList().AddStringAttribute(attrdef, lua_tostring(l, -1));
                    }
                }
                lua_pop(l, 1);
            }
        } 
        if (entity != nullptr && !GiveItemToPlayer(player, entity, noRemove, forceGive, name)) {
            entity->Remove();
            entity = nullptr;
        }
        if (entity != nullptr) {
            LEntityAlloc(l, entity);
        }
        else {
            lua_pushnil(l);
        }
        return 1;
    }
    
    int LEntityFireOutput(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        variant_t value;
        LToVariant(l, 3, value);
        auto activator = LEntityGetOptional(l, 4);
        auto delay = luaL_optnumber(l, 5, 0);
        entity->FireNamedOutput(name,value, activator, entity, delay);
        return 0;
    }

    int LEntitySetFakeProp(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        variant_t value;
        LToVariant(l, 3, value);
        
        auto mod = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
        mod->props[name] = {value, value};
        return 0;
    }

    int LEntityResetFakeProp(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        variant_t value;
        LToVariant(l, 3, value);
        
        auto mod = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
        mod->props.erase(name);
        return 0;
    }

    int LEntityGetFakeProp(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        auto name = luaL_checkstring(l, 2);
        
        auto mod = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::FakePropModule>("fakeprop");
        LFromVariant(l, mod->props[name].first);
        return 1;
    }

    int LEntityAddEffects(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        entity->AddEffects(luaL_checknumber(l, 2));
        return 0;
    }

    int LEntityRemoveEffects(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        entity->RemoveEffects(luaL_checknumber(l, 2));
        return 0;
    }

    int LEntityIsEffectActive(lua_State *l)
    {
        auto entity = LEntityGetNonNull(l, 1);
        lua_pushboolean(l, entity->IsEffectActive(luaL_checknumber(l, 2)));
        
        return 1;
    }

    int LEntitySnapEyeAngles(lua_State *l)
    {
        auto entity = LPlayerGetNonNull(l, 1);
        auto vec = LAngleGetCheck(l, 2);
        entity->SnapEyeAngles(*vec);
        
        return 0;
    }

    
    int LEntityPrintHud(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        luaL_checktype(l, 2, LUA_TTABLE);
        std::string str;
        ConstructPrint(l, 3, str);

        hudtextparms_t textparam;
        lua_getfield(l, 2, "channel"); textparam.channel = luaL_optinteger(l, -1, 4) % 6; lua_pop(l,1);
        lua_getfield(l, 2, "x"); textparam.x = luaL_optnumber(l, -1, -1); lua_pop(l,1);
        lua_getfield(l, 2, "y"); textparam.y = luaL_optnumber(l, -1, -1); lua_pop(l,1);
        lua_getfield(l, 2, "effect"); textparam.effect = luaL_optinteger(l, -1, 0); lua_pop(l,1);
        lua_getfield(l, 2, "r1"); textparam.r1 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "r2"); textparam.r2 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "g1"); textparam.g1 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "g2"); textparam.g2 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "b1"); textparam.b1 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "b2"); textparam.b2 = luaL_optinteger(l, -1, 255); lua_pop(l,1);
        lua_getfield(l, 2, "a1"); textparam.a1 = luaL_optinteger(l, -1, 0); lua_pop(l,1);
        lua_getfield(l, 2, "a2"); textparam.a2 = luaL_optinteger(l, -1, 0); lua_pop(l,1);
        lua_getfield(l, 2, "fadeinTime"); textparam.fadeinTime = luaL_optnumber(l, -1, 0); lua_pop(l,1);
        lua_getfield(l, 2, "fadeoutTime"); textparam.fadeoutTime = luaL_optnumber(l, -1, 0); lua_pop(l,1);
        lua_getfield(l, 2, "holdTime"); textparam.holdTime = luaL_optnumber(l, -1, 9999); lua_pop(l,1);
        lua_getfield(l, 2, "fxTime"); textparam.fxTime = luaL_optnumber(l, -1, 0); lua_pop(l,1);
        UTIL_HudMessage(player, textparam, str.c_str());
        
        return 0;
    }

    enum PrintTarget
    {
        PRINT_TARGET_CONSOLE,
        PRINT_TARGET_CHAT,
        PRINT_TARGET_CENTER,
        PRINT_TARGET_HINT,
        PRINT_TARGET_RIGHT,
        PRINT_TARGET_COUNT
    };

    int LEntityPrintTo(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        PrintTarget target = (PrintTarget) luaL_checknumber(l, 2);
        luaL_argcheck(l, target >= PRINT_TARGET_CONSOLE && target < PRINT_TARGET_COUNT, 3, "Print target out of bounds");
        std::string str;
        ConstructPrint(l, 3, str);
        switch (target) {
            case PRINT_TARGET_CONSOLE: ClientMsg(player,"%s\n", str.c_str()); return 0;
            case PRINT_TARGET_CHAT: PrintToChat(str.c_str(), player); return 0;
            case PRINT_TARGET_CENTER: gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, str.c_str()); return 0;
            case PRINT_TARGET_HINT: gamehelpers->HintTextMsg(ENTINDEX(player), str.c_str());
            case PRINT_TARGET_RIGHT: {
                CRecipientFilter filter;
                filter.AddRecipient(player);
                bf_write *msg = engine->UserMessageBegin(&filter, usermessages->LookupUserMessage("KeyHintText"));
                msg->WriteByte(1);
                msg->WriteString(str.c_str());
                engine->MessageEnd();
                return 0;
            }
        }
        
        return 0;
    }

    
    class LuaMenuHandler : public IMenuHandler, public AutoList<LuaMenuHandler>
    {
    public:

        LuaMenuHandler(CBasePlayer *player, LuaState *state) : IMenuHandler(), player(player), state(state) 
        {
            tableRef = luaL_ref(state->GetState(), LUA_REGISTRYINDEX);
        }
        virtual ~LuaMenuHandler()
        {
            luaL_unref(state->GetState(), LUA_REGISTRYINDEX, tableRef);
        }

        void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item)
        {
            if (invalid) return;

            const char *info = menu->GetItemInfo(item, nullptr);

            lua_rawgeti(state->GetState(), LUA_REGISTRYINDEX, tableRef);
            if (lua_getfield(state->GetState(), -1, "onSelect") != LUA_TNIL) {
                LEntityAlloc(state->GetState(), player.Get());
                lua_pushinteger(state->GetState(), item + 1);
                lua_pushstring(state->GetState(), info);
                state->Call(3, 0);
            }
            else {
                lua_pop(state->GetState(), 1);
            }
            lua_pop(state->GetState(), 1);
        }

        virtual void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
		{
            if (invalid) return;

            lua_rawgeti(state->GetState(), LUA_REGISTRYINDEX, tableRef);
            if (lua_getfield(state->GetState(), -1, "onCancel") != LUA_TNIL) {
                LEntityAlloc(state->GetState(), player.Get());
                lua_pushinteger(state->GetState(), reason);
                state->Call(2, 0);
            }
            else {
                lua_pop(state->GetState(), 1);
            }
            lua_pop(state->GetState(), 1);
		}

        virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
            //menu->Destroy(false);
            HandleSecurity sec(g_Ext.GetIdentity(), g_Ext.GetIdentity());
			handlesys->FreeHandle(menu->GetHandle(), &sec);
		}

        void OnMenuDestroy(IBaseMenu *menu) {
            delete this;
        }

        CHandle<CBasePlayer> player;
        LuaState *state;
        int tableRef;
        bool invalid = false;
    };

    int LEntityDisplayMenu(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        if (!player->IsRealPlayer()) return 0;

        luaL_checktype(l, 2, LUA_TTABLE);
        lua_pushvalue(l, 2);
        LuaMenuHandler *handler = new LuaMenuHandler(player, cur_state);
        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler, g_Ext.GetIdentity());

        int len = lua_rawlen(l, 2);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(l, 2, i);
            // Advanced table entry
            if (lua_type(l, -1) == LUA_TTABLE) {
                lua_getfield(l, -1, "text");
                const char *name = luaL_optstring(l, -1, "");
                lua_pop(l, 1);

                lua_getfield(l, -1, "value");
                const char *value = luaL_optstring(l, -1, "");
                lua_pop(l, 1);

                lua_getfield(l, -1, "disabled");
                bool disabled = lua_toboolean(l, -1);
                lua_pop(l, 1);

                if (name != nullptr) {
                    ItemDrawInfo info1(name, disabled ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
                    menu->AppendItem(value, info1);
                }

                lua_pop(l, 1);
            }
            // Simple string entry
            else {
                const char *name = lua_tostring(l, -1);
                if (name != nullptr) {
                    bool enabled = name[0] != '!';
                    ItemDrawInfo info1(enabled ? name : name + 1, enabled ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
                    menu->AppendItem(name, info1);
                }
                lua_pop(l, 1);
            }
        }
        lua_getfield(l, 2, "timeout");
        int timeout = luaL_optnumber(l, -1, 0);

        lua_getfield(l, 2, "title");
        menu->SetDefaultTitle(luaL_optstring(l, -1, "Menu"));

        lua_getfield(l, 2, "itemsPerPage");
        if (!lua_isnil(l, -1)) {
            menu->SetPagination(lua_tointeger(l, -1));
        }

        lua_getfield(l, 2, "flags");
        menu->SetMenuOptionFlags(luaL_optinteger(l, -1, MENUFLAG_BUTTON_EXIT));

        menu->Display(ENTINDEX(player), timeout);
        return 0;
    }

    int LEntityHideMenu(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(player), false);
        return 0;
    }

    int LEntityGetUserId(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        lua_pushinteger(l, player->GetUserID());
        return 1;
    }

    int LFindEntityByName(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        auto prevEntity = lua_gettop(l) < 2 ? nullptr : LEntityGetCheck(l, 2)->Get();
        LEntityAlloc(l, servertools->FindEntityByName(prevEntity, name));
        return 1;
    }

    int LFindEntityByClassname(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        auto prevEntity = lua_gettop(l) < 2 ? nullptr : LEntityGetCheck(l, 2)->Get();
        LEntityAlloc(l, servertools->FindEntityByClassname(prevEntity, name));
        return 1;
    }

    int LFindAllEntityByName(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        lua_newtable(l);
        int idx = 1;
        for(CBaseEntity *entity = nullptr ; (entity = servertools->FindEntityByName(entity, name)) != nullptr; ) {
            LEntityAlloc(l, entity);
            lua_rawseti (l, -2, idx);
            idx++;
        }
        return 1;
    }

    int LFindAllEntityByClassname(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        lua_newtable(l);
        int idx = 1;
        for(CBaseEntity *entity = nullptr ; (entity = servertools->FindEntityByClassname(entity, name)) != nullptr; ) {
            LEntityAlloc(l, entity);
            lua_rawseti (l, -2, idx);
            idx++;
        }
        return 1;
    }

    int LFindAllEntity(lua_State *l)
    {
        lua_createtable(l, gEntList->m_iNumEnts, 0);
        int idx = 1;
        for (const CEntInfo *pInfo = gEntList->FirstEntInfo(); pInfo != nullptr; pInfo = pInfo->m_pNext) {
            CBaseEntity *entity = (CBaseEntity *) pInfo->m_pEntity;
            if (entity != nullptr) {
                LEntityAlloc(l, entity);
                lua_rawseti (l, -2, idx);
                idx++;
            }
        }
        return 1;
    }

    int LFindAllPlayers(lua_State *l)
    {
        lua_newtable(l);
        int idx = 1;
        ForEachTFPlayer([&](CTFPlayer *player) {
            LEntityAlloc(l, player);
            lua_rawseti(l, -2, idx);
            idx++;
        });

        return 1;
    }

    class CLuaEnumerator : public IPartitionEnumerator
    {
    public:
        CLuaEnumerator(lua_State *l) : l(l) {};

        // This gets called	by the enumeration methods with each element
        // that passes the test.
        virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity ) {
            CBaseEntity *pEntity = static_cast<IServerUnknown *>(pHandleEntity)->GetBaseEntity();
            if ( pEntity )
            {
                LEntityAlloc(l, pEntity);
                lua_rawseti (l, -2, idx);
                idx++;
            }
            return ITERATION_CONTINUE;
        }
        lua_State *l;
        int idx = 1;
    };

    int LFindAllEntityInBox(lua_State *l)
    {
        auto min = LVectorGetCheck(l, 1);
        auto max = LVectorGetCheck(l, 2);

        CLuaEnumerator iter = CLuaEnumerator(l);

        lua_newtable(l);
		partition->EnumerateElementsInBox(PARTITION_ENGINE_NON_STATIC_EDICTS, *min, *max, false, &iter);
        return 1;
    }

    int LFindAllEntityInSphere(lua_State *l)
    {
        auto center = LVectorGetCheck(l, 1);
        auto radius = luaL_checknumber(l, 2);

        CLuaEnumerator iter = CLuaEnumerator(l);

        lua_newtable(l);
		partition->EnumerateElementsInSphere(PARTITION_ENGINE_NON_STATIC_EDICTS, *center, radius, false, &iter);
        return 1;
    }

    int LEntityFirst(lua_State *l)
    {
        LEntityAlloc(l, servertools->FirstEntity());
        return 1;
    }

    int LEntityNext(lua_State *l)
    {
        LEntityAlloc(l, servertools->NextEntity(LEntityGetCheck(l,1)->Get()));
        return 1;
    }

    int LPlayerByUserID(lua_State *l)
    {
        LEntityAlloc(l, UTIL_PlayerByUserId(luaL_checkinteger(l, 1)));
        return 1;
    }

    void LCreateTimer(lua_State *l)
    {

        float delay = luaL_checknumber(l, 1);
        
        luaL_checktype(l, 2, LUA_TFUNCTION);
        lua_pushvalue(l,2);
        int func = luaL_ref(l, LUA_REGISTRYINDEX);
        int repeats = lua_gettop(l) >= 3 ? luaL_checknumber(l,3) : 1;

        lua_pushvalue(l,4);
        int param = lua_gettop(l) >= 4 ? luaL_ref(l, LUA_REGISTRYINDEX) : 0;

        lua_pushnumber(l, cur_state->AddTimer(delay, repeats, func, param));
    }
    
    int LTimerSimple(lua_State *l)
    {
        float delay = luaL_checknumber(l, 1);
        luaL_checktype(l, 2, LUA_TFUNCTION);
        lua_pushvalue(l,2);
        int func = luaL_ref(l, LUA_REGISTRYINDEX);

        lua_pushnumber(l, cur_state->AddTimer(delay, 1, func, 0));
        return 1;
    }

    int LTimerCreate(lua_State *l)
    {
        LCreateTimer(l);
        return 1;
    }

    int LTimerStop(lua_State *l)
    {
        int id = luaL_checkinteger(l, 1);
        if (!cur_state->StopTimer(id)) {
            luaL_error(l, "Timer with id %d is not valid\n", id);
        }
        return 0;
    }

    class LuaTraceFilter : public CTraceFilterSimple
    {
    public:
        LuaTraceFilter(lua_State *l, IHandleEntity *entity, int collisionGroup) : CTraceFilterSimple( entity, collisionGroup)
        {
            if (lua_istable(l, -1)) {
                int len = lua_rawlen(l, -1);
                for(int i = 1; i <= len; i++) {
                    lua_rawgeti(l, -1, i);
                    auto entity = LEntityGetOptional(l, -1);

                    if (entity != nullptr) {
                        list.push_back(entity);
                    }
                    lua_pop(l, 1);
                }
            }
            this->hasFunction = lua_isfunction(l, -1);
        }
        virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask) {
            if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask)) {
                if (!list.empty() && std::find(list.begin(), list.end(), pHandleEntity) != list.end()) {
                    return false;
                }
                if (hasFunction) {
                    LEntityAlloc(l, EntityFromEntityHandle(pHandleEntity));
                    lua_call(l, 1, 1);
                    bool hit = lua_toboolean(l, -1);
                    lua_pop(l, 1);
                    return hit;
                }
            }
            return false;
        }
        std::vector<IHandleEntity *> list;
        lua_State *l;
        bool hasFunction;
    };

    int LTraceLine(lua_State *l)
    {
        lua_getfield(l, 1, "start");
        auto startptrentity = LEntityGetOptional(l, -1);
        auto startptrvector = LVectorGetNoCheck(l, -1);
        Vector start(0,0,0);
        if (startptrentity != nullptr) {
            start = startptrentity->EyePosition();
        }
        else if (startptrvector != nullptr) {
            start = *startptrvector;
        }

        lua_getfield(l, 1, "endpos");
        auto endptr = LVectorGetNoCheck(l, -1);
        Vector end(0,0,0);
        if (endptr != nullptr) {
            end = *endptr;
        }
        else {
            QAngle angles(0,0,0);
            lua_getfield(l, 1, "distance");
            float distance = lua_isnil(l, -1) ? 8192 : lua_tonumber(l, -1);
            
            lua_getfield(l, 1, "angles");
            auto anglesptr = LAngleGetNoCheck(l, -1);
            if (anglesptr != nullptr) {
                angles = *anglesptr;
            }
            else if (startptrentity != nullptr) {
                angles = startptrentity->EyeAngles();
            }
            Vector fwd;
            AngleVectors(angles, &fwd);
            end = start + fwd * distance;
        }

        lua_getfield(l, 1, "mask");
        int mask = lua_isnil(l, -1) ? MASK_SOLID : lua_tointeger(l, -1);
        lua_getfield(l, 1, "collisiongroup");
        int collisiongroup = lua_isnil(l, -1) ? COLLISION_GROUP_NONE : lua_tointeger(l, -1);

        //lua_getfield(l, 1, "filter");
        //
        //

        trace_t trace;
        lua_getfield(l, 1, "mins");
        auto *minsptr = LVectorGetNoCheck(l, -1);
        lua_getfield(l, 1, "maxs");
        auto *maxsptr = LVectorGetNoCheck(l, -1);
        
        
        lua_getfield(l, 1, "filter");
        CBaseEntity *filterEntity = lua_isnil(l, -1) ? startptrentity : LEntityGetOptional(l, -1);
        LuaTraceFilter filter(l, filterEntity, collisiongroup);
        if (minsptr == nullptr || maxsptr == nullptr) {
	        UTIL_TraceLine(start, end, mask, &filter, &trace);
        }
        else {
	        UTIL_TraceHull(start, end, *minsptr, *maxsptr, mask, &filter, &trace);
        }
            

        lua_newtable(l);
        LEntityAlloc(l, trace.m_pEnt);
        lua_setfield(l, -2, "Entity");
        lua_pushnumber(l, trace.fraction);
        lua_setfield(l, -2, "Fraction");
        lua_pushnumber(l, trace.fractionleftsolid);
        lua_setfield(l, -2, "FractionLeftSolid");
        lua_pushboolean(l, trace.DidHit());
        lua_setfield(l, -2, "Hit");
        lua_pushinteger(l, trace.hitbox);
        lua_setfield(l, -2, "HitBox");
        lua_pushinteger(l, trace.hitgroup);
        lua_setfield(l, -2, "HitGroup");
        lua_pushboolean(l, trace.surface.flags & SURF_NODRAW);
        lua_setfield(l, -2, "HitNoDraw");
        lua_pushboolean(l, trace.DidHitNonWorldEntity());
        lua_setfield(l, -2, "HitNonWorld");
        auto normal = LVectorAlloc(l);
        *normal = trace.plane.normal;
        lua_setfield(l, -2, "HitNormal");
        auto hitpos = LVectorAlloc(l);
        *hitpos = trace.endpos;
        lua_setfield(l, -2, "HitPos");
        lua_pushboolean(l, trace.surface.flags & SURF_SKY);
        lua_setfield(l, -2, "HitSky");
        lua_pushstring(l, trace.surface.name);
        lua_setfield(l, -2, "HitTexture");
        lua_pushboolean(l, trace.DidHitWorld());
        lua_setfield(l, -2, "HitWorld");
        auto normal2 = LVectorAlloc(l);
        *normal2 = (trace.endpos - trace.startpos).Normalized();
        lua_setfield(l, -2, "Normal");
        auto start2 = LVectorAlloc(l);
        *start2 = trace.startpos;
        lua_setfield(l, -2, "StartPos");
        lua_pushboolean(l, trace.startsolid);
        lua_setfield(l, -2, "StartSolid");
        lua_pushboolean(l, trace.allsolid);
        lua_setfield(l, -2, "AllSolid");
        lua_pushinteger(l, trace.surface.flags);
        lua_setfield(l, -2, "SurfaceFlags");
        lua_pushinteger(l, trace.dispFlags);
        lua_setfield(l, -2, "DispFlags");
        lua_pushinteger(l, trace.contents);
        lua_setfield(l, -2, "Contents");
        lua_pushinteger(l, trace.surface.surfaceProps);
        lua_setfield(l, -2, "SurfaceProps");
        lua_pushinteger(l, trace.physicsbone);
        lua_setfield(l, -2, "PhysicsBone");
        return 1;
    }

    int LUtilPrintToConsoleAll(lua_State *l)
    {
        std::string str;
        ConstructPrint(l, 1, str);
        ClientMsgAll("%s\n", str.c_str());
        return 0;
    }

    int LUtilPrintToConsole(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        std::string str;
        ConstructPrint(l, 2, str);
        ClientMsg(player,"%s\n", str.c_str());
        return 0;
    }

    int LUtilPrintToChatAll(lua_State *l)
    {
        std::string str;
        ConstructPrint(l, 1, str);
        PrintToChatAll(str.c_str());
        return 0;
    }

    int LUtilPrintToChat(lua_State *l)
    {
        auto player = LPlayerGetNonNull(l, 1);
        std::string str;
        ConstructPrint(l, 2, str);
        PrintToChat(str.c_str(), player);
        return 0;
    }

    int LUtilParticleEffect(lua_State *l)
    {
        auto name = luaL_checkstring(l, 1);
        auto pos = LVectorGetNoCheck(l, 2);
        auto ang = LAngleGetNoCheck(l, 3);
        auto ent = LEntityGetOptional(l, 4);
        DispatchParticleEffect(name, pos != nullptr ? *pos : vec3_origin, ang != nullptr ? *ang : vec3_angle, ent);
        return 0;
    }

    int LCurTime(lua_State *l)
    {
        lua_pushnumber(l, gpGlobals->curtime);
        return 1;
    }

    int LTickCount(lua_State *l)
    {
        lua_pushinteger(l, gpGlobals->tickcount);
        return 1;
    }

    int LGetMapName(lua_State *l)
    {
        lua_pushstring(l, STRING(gpGlobals->mapname));
        return 1;
    }

    struct EventCallback
    {   
        LuaState *state;
        std::string name;
        int func;
    };
    std::vector<EventCallback> event_callbacks;

    int LAddEventCallback(lua_State *l)
    {
        const char *name = luaL_checkstring(l, 1);
        luaL_argcheck(l, strlen(name) > 0, 1, "event name empty");
        luaL_checktype(l,2, LUA_TFUNCTION);
        lua_pushvalue(l,2);
        int func = luaL_ref(l, LUA_REGISTRYINDEX);
        
        event_callbacks.push_back({cur_state, name, func});
        lua_pushinteger(l, func);
        return 1;
    }

    int LRemoveEventCallback(lua_State *l)
    {
        int func = luaL_checkinteger(l, 1);
        RemoveIf(event_callbacks, [&](auto &callback){ 
            if (callback.state == cur_state && callback.func == func) {
                luaL_unref(l, LUA_REGISTRYINDEX, func);
                return true ;
            }
            return false ;
        });

        return 0;
    }

    static const struct luaL_Reg vectorlib_f [] = {
        {"__call", LVectorNew},
        {nullptr, nullptr},
    };

    static const struct luaL_Reg vectorlib_m [] = {
        {"Length", LVectorLength},
        {"Distance", LVectorDistance},
        {"Cross", LVectorCross},
        {"Dot", LVectorDot},
        {"ToAngles", LVectorToAngles},
        {"GetForward", LVectorGetForward},
        {"GetAngleVectors", LVectorGetAngleVectors},
        {"Normalize", LVectorNormalize},
        {"Rotate", LVectorRotate},
        {"Set", LVectorCopy},
        {"SetUnpacked", LVectorCopyUnpacked},
        {"__add", LVectorAdd},
        {"__sub", LVectorSubtract},
        {"__mul", LVectorMultiply},
        {"__div", LVectorDivide},
        {"__eq", LVectorEquals},
        {"__tostring", LVectorToString},
        {"__index", LVectorIndex},
        {"__newindex", LVectorSetIndex},
        {nullptr, nullptr},
    };

    static const struct luaL_Reg entitylib_f [] = {
        {"__call", LEntityNew},
        {"FindByName", LFindEntityByName},
        {"FindByClass", LFindEntityByClassname},
        {"FindAllByName", LFindAllEntityByName},
        {"FindAllByClass", LFindAllEntityByClassname},
        {"FindInBox", LFindAllEntityInBox},
        {"FindInSphere", LFindAllEntityInSphere},
        {"GetAll", LFindAllEntity},
        {"GetAllPlayers", LFindAllPlayers},
        {"Create", LEntityNew},
        {"GetFirstEntity", LEntityFirst},
        {"GetNextEntity", LEntityNext},
        {"AddCreateCallback", LEntityAddCreateCallback},
        {"RemoveCreateCallback", LEntityRemoveCreateCallback},
        {"GetPlayerByUserId", LPlayerByUserID},
        {nullptr, nullptr},
    };

    static const struct luaL_Reg entitylib_m [] = {
        {"IsValid", LEntityIsValid},
        {"DispatchSpawn", LEntitySpawn},
        {"Activate", LEntityActivate},
        {"Remove", LEntityRemove},
        {"AcceptInput", LEntityAcceptInput},
        {"GetNetIndex", LEntityGetNetworkIndex},
        {"GetHandleIndex", LEntityGetHandleIndex},
        {"GetPlayerItemBySlot", LEntityGetPlayerItemBySlot},
        {"GetPlayerItemByName", LEntityGetPlayerItemByName},
        {"GetAttributeValue", LEntityGetItemAttribute},
        {"SetAttributeValue", LEntitySetItemAttribute},
        {"PrintToChat", LUtilPrintToChat},
        {"PrintToConsole", LUtilPrintToConsole},
        {"GetName", LEntityGetName},
        {"SetName", LEntitySetName},
        {"GetPlayerName", LEntityGetPlayerName},
        {"IsAlive", LEntityIsAlive},
        {"IsPlayer", LEntityIsPlayer},
        {"IsNPC", LEntityIsNPC},
        {"IsBot", LEntityIsBot},
        {"IsRealPlayer", LEntityIsRealPlayer},
        {"IsObject", LEntityIsObject},
        {"IsWeapon", LEntityIsWeapon},
        {"IsCombatCharacter", LEntityIsCombatCharacter},
        {"IsWearable", LEntityIsWearable},
        {"IsWorld", LEntityIsWorld},
        {"AddCallback", LEntityAddCallback},
        {"RemoveCallback", LEntityRemoveCallback},
        {"DumpProperties", LEntityDumpProperties},
        {"DumpInputs", LEntityDumpInputs},
        {"TakeDamage", LEntityTakeDamage},
        {"AddHealth", LEntityTakeHealth},
        {"AddCond", LEntityAddCond},
        {"RemoveCond", LEntityRemoveCond},
        {"InCond", LEntityInCond},
        {"GetConditionProvider", LEntityGetCondProvider},
        {"StunPlayer", LEntityStunPlayer},
        {"GetAbsOrigin", LEntityGetAbsOrigin},
        {"SetAbsOrigin", LEntitySetAbsOrigin},
        {"GetAbsAngles", LEntityGetAbsAngles},
        {"SetAbsAngles", LEntitySetAbsAngles},
        {"Teleport", LEntityTeleport},
        {"GiveItem", LEntityCreateItem},
        {"FireOutput", LEntityFireOutput},
        {"SetFakeSendProp", LEntitySetFakeProp},
        {"ResetFakeSendProp", LEntityResetFakeProp},
        {"GetFakeSendProp", LEntityGetFakeProp},
        {"AddEffects", LEntityAddEffects},
        {"RemoveEffects", LEntityRemoveEffects},
        {"IsEffectActive", LEntityIsEffectActive},
        {"ShowHudText", LEntityPrintHud},
        {"Print", LEntityPrintTo},
        {"DisplayMenu", LEntityDisplayMenu},
        {"HideMenu", LEntityHideMenu},
        {"SnapEyeAngles", LEntitySnapEyeAngles},
        {"GetUserId", LEntityGetUserId},
        {"__eq", LEntityEquals},
        {"__tostring", LEntityToString},
        {"__index", LEntityGetProp},
        {"__newindex", LEntitySetProp},
        
        {nullptr, nullptr},
    };

    static const struct luaL_Reg timerlib_f [] = {
        
        {"Create", LTimerCreate},
        {"Simple", LTimerSimple},
        {"Stop", LTimerStop},
        {nullptr, nullptr},
    };

    static const struct luaL_Reg utillib_f [] = {
        
        {"Trace", LTraceLine},
        {"PrintToConsoleAll", LUtilPrintToConsoleAll},
        {"PrintToConsole", LUtilPrintToConsole},
        {"PrintToChatAll", LUtilPrintToChatAll},
        {"PrintToChat", LUtilPrintToChat},
        {"ParticleEffect", LUtilParticleEffect},
        {nullptr, nullptr},
    };

    static const struct luaL_Reg proplib_m [] = {
        {"__index", LPropGet},
        {"__newindex", LPropSet},
        {"__len", LPropGetN},
        {nullptr, nullptr},
    };

    THINK_FUNC_DECL(ScriptModuleTick)
    {
        
        this->SetNextThink(gpGlobals->curtime + 6.0f, "ScriptModuleTick");
    }

    LuaState::LuaState() 
    {
        l = luaL_newstate();
        luaL_openlibs(l);

        // Blacklist some stuff
        lua_pushnil(l);
        lua_setglobal(l, "io");
        lua_pushnil(l);
        lua_setglobal(l, "package");
        lua_pushnil(l);
        lua_setglobal(l, "loadfile");
        lua_pushnil(l);
        lua_setglobal(l, "dofile");
        lua_pushnil(l);
        lua_setglobal(l, "load");
        lua_pushnil(l);
        lua_setglobal(l, "require");

        lua_getglobal(l, "os");
        lua_pushnil(l);
        lua_setfield(l, -2, "execute");
        lua_pushnil(l);
        lua_setfield(l, -2, "getenv");
        lua_pushnil(l);
        lua_setfield(l, -2, "remove");
        lua_pushnil(l);
        lua_setfield(l, -2, "rename");
        lua_pushnil(l);
        lua_setfield(l, -2, "setlocale");
        lua_pushnil(l);
        lua_setfield(l, -2, "tmpname");
        lua_pushnil(l);
        lua_setfield(l, -2, "exit");
        
        // Override print
        lua_register(l, "print", LPrint);

        /* Vector */
        luaL_newmetatable(l, "vector");
        luaL_setfuncs(l, vectorlib_m, 0);
        m_pVectorMeta = vector_meta_ptr = lua_topointer(l, -1);
        m_iVectorMeta = vector_meta = luaL_ref(l, LUA_REGISTRYINDEX);

        luaL_newmetatable(l, "entity");
        luaL_setfuncs(l, entitylib_m, 0);
        m_pEntityMeta = entity_meta_ptr = lua_topointer(l, -1);
        m_iEntityMeta = entity_meta = luaL_ref(l, LUA_REGISTRYINDEX);

        luaL_newmetatable(l, "prop");
        luaL_setfuncs(l, proplib_m, 0);
        m_iPropMeta = prop_meta = luaL_ref(l, LUA_REGISTRYINDEX);
        //lua_rawsetp(l, LUA_REGISTRYINDEX, (void*)entity_meta);

        //lua_pushstring(l, "__index");
        //lua_pushvalue(l, -2);
        //lua_settable(l, -3);
        
        //lua_pushcfunction(l, LVectorSetIndex);
        //lua_settable(l, -3);

        //lua_pushstring(l, "");
        //lua_pushcfunction(l, LVectorIndex);
        //lua_settable(l, -3);
        
        lua_register(l, "Vector", LVectorNew);
        lua_register(l, "Entity", LEntityNew);
        lua_register(l, "Timer", LTimerCreate);
        lua_register(l, "StopTimer", LTimerStop);
        lua_register(l, "CurTime", LCurTime);
        lua_register(l, "GetMapName", LGetMapName);
        lua_register(l, "TickCount", LTickCount);
        lua_register(l, "IsValid", LIsValid);
        lua_register(l, "AddEventCallback", LAddEventCallback);
        lua_register(l, "RemoveEventCallback", LRemoveEventCallback);
        

        luaL_newlib(l, entitylib_f);
        //lua_newtable(l);
        //luaL_setfuncs(l, entitylib_f, 0);
        lua_setglobal(l, "ents");

        luaL_newlib(l, timerlib_f);
        //lua_newtable(l);
        //luaL_setfuncs(l, timerlib_f, 0);
        lua_setglobal(l, "timer");

        luaL_newlib(l, utillib_f);
        //lua_newtable(l);
        //luaL_setfuncs(l, utillib_f, 0);
        lua_setglobal(l, "util");

        lua_newtable(l);
        m_iEntityTableStorage = entity_table_store = luaL_ref(l, LUA_REGISTRYINDEX);

        if (filesystem != nullptr)
            DoFile("scripts/globals.lua", true);
    }

    LuaState::~LuaState() {
        for (auto menu : LuaMenuHandler::List()) {
            menu->invalid = true;
        }

        for (auto entity : callbackEntities) {
            auto luaModule = entity->GetEntityModule<LuaEntityModule>("luaentity");
            if (luaModule != nullptr) {
                luaModule->LuaStateRemoved(this);
            }
        }
        RemoveIf(entity_create_callbacks, [&](auto &callback){
            return callback.state == this;
        });
        RemoveIf(event_callbacks, [&](auto &callback){
            return callback.state == this;
        });
        
        lua_close(l);
    }

    double script_exec_time = 0.0;
    double script_exec_time_tick = 0.0;
    bool script_exec_time_guard = false;
    void LuaState::DoString(const char *str, bool execute) {
        VPROF_BUDGET("LuaState::DoString", "Lua");
        CFastTimer timer;
        timer.Start();
        SwitchState();
        {
            int err = luaL_loadstring(l, str);
            if (err) {
                const char *errbuf = lua_tostring(l, -1);
                SendWarningConsoleMessageToAdmins("%s\n", errbuf);
            }
            else if (execute) {
                err = lua_pcall(l, 0, LUA_MULTRET, 0);
                
                if (err) {
                    const char *errbuf = lua_tostring(l, -1);
                    SendWarningConsoleMessageToAdmins("%s\n", errbuf);
                }
            }
        }
        lua_settop(l,0);
        timer.End();
        script_exec_time += timer.GetDuration().GetSeconds();
        script_exec_time_tick += timer.GetDuration().GetSeconds();
    }

    void LuaState::DoFile(const char *path, bool execute) {
        VPROF_BUDGET("LuaState::DoFile", "Lua");
        CFastTimer timer;
        timer.Start();
        SwitchState();

        FileHandle_t f = filesystem->Open(path, "rb", "GAME");
        if (f == nullptr) {
            f = filesystem->Open(CFmtStr("scripts/%s",path), "rb", "GAME");
        }
        if (f == nullptr) {
            f = filesystem->Open(CFmtStr("scripts/population/%s",path), "rb", "GAME");
        }
        if (f == nullptr) {
            SendWarningConsoleMessageToAdmins("Cannot find lua script file %s\n", path);
            return;
        }
        int fileSize = filesystem->Size(f);
        if (fileSize > 200000) {
            SendWarningConsoleMessageToAdmins("Lua script file %s is too large\n", path);
            return;
        }
        uint bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize(f, fileSize + 2);
        char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer(f, bufSize);
        // read into local buffer
        bool bRetOK = (((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f) != 0);
        
        if (bRetOK) {
            int err = luaL_loadbuffer(l, buffer, fileSize, path);
            if (err) {
                const char *errbuf = lua_tostring(l, -1);
                SendWarningConsoleMessageToAdmins("%s\n", errbuf);
            }
            else if (execute) {
                err = lua_pcall(l, 0, LUA_MULTRET, 0);
                
                if (err) {
                    const char *errbuf = lua_tostring(l, -1);
                    SendWarningConsoleMessageToAdmins("%s\n", errbuf);
                }
            }
        }
        else {
            SendWarningConsoleMessageToAdmins("Fail reading buffer of lua script file %s\n", path);
            return;
        }
        lua_settop(l,0);
        timer.End();
        script_exec_time += timer.GetDuration().GetSeconds();
        script_exec_time_tick += timer.GetDuration().GetSeconds();
    }

    void LuaState::SwitchState()
    {
        cur_state = this;
        vector_meta = m_iVectorMeta;
        vector_meta_ptr = m_pVectorMeta;
        entity_meta = m_iEntityMeta;
        entity_meta_ptr = m_pEntityMeta;
        entity_table_store = m_iEntityTableStorage;
        prop_meta = m_iPropMeta;
    }

    void LuaState::CallGlobal(const char *str, int numargs) {
        VPROF_BUDGET("LuaState::CallGlobal", "Lua");
        CFastTimer timer;
        timer.Start();
        
        SwitchState();
        {
            lua_getglobal(l, str);
            int err = lua_pcall(l, numargs, 0, 0);
            if (err) {
                const char *errstr = lua_tostring(l, -1);
                SendWarningConsoleMessageToAdmins("%s\n", errstr);
            }
        }
        stackDump(l);
        lua_settop(l,0);

        timer.End();
        script_exec_time += timer.GetDuration().GetSeconds();
        script_exec_time_tick += timer.GetDuration().GetSeconds();
    }


    int LuaState::Call(int numargs, int numret) {
        VPROF_BUDGET("LuaState::Call", "Lua");
        //TIME_SCOPE2(call);
        CFastTimer timer;
        timer.Start();
        SwitchState();

        int err = lua_pcall(l, numargs, numret, 0);
        if (err) {
            const char *errstr = lua_tostring(l, -1);
            SendWarningConsoleMessageToAdmins("%s\n", errstr);
        }

        timer.End();
        script_exec_time += timer.GetDuration().GetSeconds();
        script_exec_time_tick += timer.GetDuration().GetSeconds();
        return err;
    }

    bool LuaState::CheckGlobal(const char* name) {
        int type = lua_getglobal(l, name);
        if (type != LUA_TNIL) {
            return true;
        }
        lua_pop(l, -1);
        return false;
    }
    void LuaState::UpdateTimers() {

        VPROF_BUDGET("LuaState::UpdateTimers", "Lua");
        SwitchState();

        this->m_bTimerLoop = true;
        for (auto it = timers.begin(); it != timers.end(); it++) {
            auto &timer = *it;

            if (!timer.m_bDestroyed && timer.m_flNextCallTime < gpGlobals->curtime) {
                lua_rawgeti(l, LUA_REGISTRYINDEX, timer.m_iRefFunc);
                if (timer.m_iRefParam != 0)
                    lua_rawgeti(l, LUA_REGISTRYINDEX, timer.m_iRefParam);

                int err = Call(timer.m_iRefParam != 0 ? 1 : 0, 1);
                bool stop = false;
                if (timer.m_iRepeats > 0) {
                    if (--timer.m_iRepeats <= 0) {
                        stop = true;
                    }
                }
                if (!lua_isnil(l, -1) && !lua_toboolean(l, -1)) {
                    stop = true;
                }
                if (!stop) {
                    lua_settop(l,0);
                    timer.m_flNextCallTime = gpGlobals->curtime + timer.m_flDelay;
                }
                else {
                    timer.m_bDestroyed = true;
                    // timer.Destroy(l);
                    // it = timers.erase(it);
                    // lua_settop(l,0);

                    // if (timers.empty())
                    //     AllTimersRemoved();
                }
            }
        }
        this->m_bTimerLoop = false;
        for (auto it = timers.begin(); it != timers.end();) {
            if (it->m_bDestroyed) {
                it = DestroyTimer(it);
            }
            else {
                it++;
            }
        }


    }

    int LuaState::AddTimer(float delay, int repeats, int reffunc, int refparam) {
        m_iNextTimerID++;
        if (timers.empty())
            TimerAdded();
        timers.emplace_back(m_iNextTimerID, delay, repeats, reffunc, refparam);
        return m_iNextTimerID;
    }

    std::list<LuaTimer>::iterator LuaState::DestroyTimer(std::list<LuaTimer>::iterator it)
    {
        it->Destroy(l);
        if (timers.empty())
            AllTimersRemoved();
        return timers.erase(it);
        
    }

    bool LuaState::StopTimer(int id) {
        for (auto it = timers.begin(); it != timers.end(); it++) {
            if (it->m_iID == id) {
                it->m_bDestroyed = true;

                if (!this->m_bTimerLoop) {
                    this->DestroyTimer(it);
                }

                return true;
            }
        }
        return false;
    }

    void LuaState::EntityDeleted(CBaseEntity *entity) {
        callbackEntities.erase(entity);
        
        lua_rawgeti(l, LUA_REGISTRYINDEX, m_iEntityTableStorage);
        lua_pushnil(l);
        lua_rawsetp(l, -2, entity);
        lua_pop(l, 1);
    }

    void LuaState::EntityCallbackAdded(CBaseEntity *entity) {
        callbackEntities.insert(entity);
    }

    void LuaState::Activate() {
        // Tell scripts about every connected player so far
        ForEachTFPlayer([&](CTFPlayer *player){
            if (this->CheckGlobal("OnPlayerConnected")) {
                LEntityAlloc(l, player);
                this->Call(1, 0);
            }
        });
    }

    void LuaTimer::Destroy(lua_State *l) {
        luaL_unref(l, LUA_REGISTRYINDEX, m_iRefFunc);
        luaL_unref(l, LUA_REGISTRYINDEX, m_iRefParam);
    }

    LuaState *state = nullptr;
    CON_COMMAND_F(sig_lua_test, "", FCVAR_NONE)
    {
        if (state == nullptr) state = new LuaState();
        state->DoString(args[1], true);
    }
    CON_COMMAND_F(sig_lua_file, "", FCVAR_NONE)
    {
        if (state == nullptr) state = new LuaState();
        state->DoFile(args[1], true);
    }
    CON_COMMAND_F(sig_lua_call, "", FCVAR_NONE)
    {
        if (state == nullptr) state = new LuaState();
        state->CallGlobal(args[1], 0);
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_Activate)
	{
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        DETOUR_MEMBER_CALL(CBaseEntity_Activate)();
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            mod->FireCallback(ON_ACTIVATE);
        }
    }

	DETOUR_DECL_STATIC(void, DispatchSpawn, CBaseEntity *entity)
	{
        DETOUR_STATIC_CALL(DispatchSpawn)(entity);
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr && !entity->IsPlayer()) {
            mod->FireCallback(ON_SPAWN);
        }
    }

	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		auto entity = DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
        if (entity != nullptr && !entity_create_callbacks.empty()) {
            for(auto &callback : entity_create_callbacks) {
                if (entity->GetClassnameString() == callback.classname || (callback.wildcard && NamesMatchCaseSensitve(STRING(callback.classname), entity->GetClassnameString()))) {
                    auto l = callback.state->GetState();
                    lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                    LEntityAlloc(l, entity);
                    lua_pushstring(l, className);
                    callback.state->Call(2, 0);
                }
            }
        }
        return entity;
	}

    RefCount rc_CBaseEntity_TakeDamage;
    
    DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, CTakeDamageInfo &info)
	{
        SCOPED_INCREMENT(rc_CBaseEntity_TakeDamage);

        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        
        int health_pre = entity->GetHealth();
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        int damage = 0;

        CTakeDamageInfo infooverride = info;
        if (mod != nullptr && !mod->callbacks[ON_DAMAGE_RECEIVED_PRE].empty()) {
            auto &callbackList = mod->callbacks[ON_DAMAGE_RECEIVED_PRE];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                DamageInfoToTable(l, infooverride);
                lua_pushvalue(l, -1);
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity);
                lua_pushvalue(l, -3);
                mod->CallCallback(callbackList, it, 2, 1);
                if (lua_toboolean(l, -1)) {
                    TableToDamageInfo(l, -2, infooverride);
                }

                lua_settop(l, 0);
            }
        }
        if (mod != nullptr && !mod->callbacks[ON_DAMAGE_RECEIVED_POST].empty()) {
            mod->lastTakeDamage = infooverride;
        }

		damage = DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(infooverride);

        if (mod != nullptr&& !mod->callbacks[ON_DAMAGE_RECEIVED_POST].empty()) {
            auto &callbackList = mod->callbacks[ON_DAMAGE_RECEIVED_POST];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity);
                DamageInfoToTable(l, mod->lastTakeDamage);
                lua_pushnumber(l, health_pre);
                mod->CallCallback(callbackList, it, 3, 0);

                lua_settop(l, 0);
            }
        }

        return damage;
	}

    DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, const CTakeDamageInfo& info)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
		auto ret = DETOUR_MEMBER_CALL(CBaseCombatCharacter_OnTakeDamage)(info);

        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            mod->lastTakeDamage = info;
        }
        return ret;
	}

    DETOUR_DECL_MEMBER(bool, CBaseEntity_AcceptInput, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
    {
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            std::function<void(LuaState *)> func = [&](LuaState *state){
                lua_pushstring(state->GetState(), szInputName);
                LFromVariant(state->GetState(), Value);
                LEntityAlloc(state->GetState(), pActivator);
                LEntityAlloc(state->GetState(), pCaller);
            };
            bool stop = false;
            std::function<void(LuaState *)> funcReturn = [&](LuaState *state){
                stop = lua_toboolean(state->GetState(), -1);
            };
            mod->FireCallback(ON_INPUT, &func, 4, &funcReturn, 1);
            if (stop) {
                return true;
            }
        }
        return DETOUR_MEMBER_CALL(CBaseEntity_AcceptInput)(szInputName, pActivator, pCaller, Value, outputID);
    }


    DETOUR_DECL_MEMBER(void, CBaseEntityOutput_FireOutput, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
    {

        CBaseEntityOutput *output = reinterpret_cast<CBaseEntityOutput *>(this);

        auto mod = pCaller != nullptr ? pCaller->GetEntityModule<LuaEntityModule>("luaentity") : nullptr;
        if (mod != nullptr) {
            
            auto datamap = pCaller->GetDataDescMap();
            const char *name = "";
            bool found = false;
            for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
                // search through all the readable fields in the data description, looking for a match
                for (int i = 0; i < dmap->dataNumFields; i++) {
                    if ((dmap->dataDesc[i].flags & FTYPEDESC_OUTPUT) && ((CBaseEntityOutput*)(((char*)pCaller) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ])) == output) {
                        name = dmap->dataDesc[i].externalName;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            
            std::function<void(LuaState *)> func = [&](LuaState *state){
                lua_pushstring(state->GetState(), name);
                LFromVariant(state->GetState(), Value);
                LEntityAlloc(state->GetState(), pActivator);
            };
            bool stop = false;
            std::function<void(LuaState *)> funcReturn = [&](LuaState *state){
                stop = lua_toboolean(state->GetState(), -1);
            };
            mod->FireCallback(ON_OUTPUT, &func, 3, &funcReturn, 1);
            if (stop) {
                return;
            }
        }
        return DETOUR_MEMBER_CALL(CBaseEntityOutput_FireOutput)(Value, pActivator, pCaller, fDelay);
    }

	DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
        DETOUR_MEMBER_CALL(CServerGameClients_ClientPutInServer)(edict, playername);
        for(auto state : LuaState::List()) {
            
            if (state->CheckGlobal("OnPlayerConnected")) {
                LEntityAlloc(state->GetState(), GetContainingEntity(edict));
                state->Call(1, 0);
            }
        }
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        DETOUR_MEMBER_CALL(CTFPlayer_UpdateOnRemove)();
        for(auto state : LuaState::List()) {
            
            if (state->CheckGlobal("OnPlayerDisconnected")) {
                LEntityAlloc(state->GetState(), player);
                state->Call(1, 0);
            }
        }
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_PlayerRunCommand, CUserCmd* cmd, IMoveHelper* moveHelper)
	{
		CTFPlayer* player = reinterpret_cast<CTFPlayer*>(this);
        int prebuttons = player->m_nButtons;
        if (prebuttons != cmd->buttons) {
            auto mod = player->GetEntityModule<LuaEntityModule>("luaentity");
            if (mod != nullptr) {
                for (int i = 0; i < 32; i++) {
                    int mask = 1 << i;
                    if ((cmd->buttons & mask) != (prebuttons & mask)) {
                        std::function<void(LuaState *)> func = [&](LuaState *state){
                            lua_pushinteger(state->GetState(), mask);
                        };
                        //bool stop = false;
                        //std::function<void(LuaState *)> funcReturn = [&](LuaState *state){
                        //    stop = lua_toboolean(state->GetState(), -1);
                        //};
                        bool release = (cmd->buttons & mask) && !(prebuttons & mask);
                        mod->FireCallback(release ? ON_KEY_PRESSED : ON_KEY_RELEASED, &func, 1);
                       //if (stop) {
                        //    cmd->buttons = release ? cmd->buttons | mask : cmd->buttons & ~(mask);
                        //}
                    }
                }
            }
        }
		DETOUR_MEMBER_CALL(CBasePlayer_PlayerRunCommand)(cmd, moveHelper);
	}

    bool profile[34] = {};
    DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player != nullptr && ENTINDEX(player) < 34) {
            if (FStrEq(args[0], "sig_lua_prof_start")) {
                profile[ENTINDEX(player)] = 1;
                return true;
            }
            if (FStrEq(args[0], "sig_lua_prof_end")) {
                profile[ENTINDEX(player)] = 0;
                return true;
            }
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}
    
    DETOUR_DECL_MEMBER(void, CBaseEntity_Event_Killed, CTakeDamageInfo &info)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        
        bool overriden = false;
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");

        if (mod != nullptr) {
            CTakeDamageInfo infooverride = info;
            auto &callbackList = mod->callbacks[ON_DEATH];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                DamageInfoToTable(l, infooverride);
                lua_pushvalue(l, -1);
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity);
                lua_pushvalue(l, -3);
                mod->CallCallback(callbackList, it, 2, 1);
                if (lua_toboolean(l, -1)) {
                    overriden = true;
                    TableToDamageInfo(l, -2, infooverride);
                }

                lua_settop(l, 0);
            }
            if(overriden) {
                DETOUR_MEMBER_CALL(CBaseEntity_Event_Killed)(infooverride);
                return;
            }
        }
		DETOUR_MEMBER_CALL(CBaseEntity_Event_Killed)(info);
	}

    DETOUR_DECL_MEMBER(void, CBaseCombatCharacter_Event_Killed, CTakeDamageInfo &info)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        
        bool overriden = false;
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");

        if (mod != nullptr) {
            CTakeDamageInfo infooverride = info;
            auto &callbackList = mod->callbacks[ON_DEATH];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                DamageInfoToTable(l, infooverride);
                lua_pushvalue(l, -1);
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity);
                lua_pushvalue(l, -3);
                mod->CallCallback(callbackList, it, 2, 1);
                if (lua_toboolean(l, -1)) {
                    overriden = true;
                    TableToDamageInfo(l, -2, infooverride);
                }

                lua_settop(l, 0);
            }
            if(overriden) {
                DETOUR_MEMBER_CALL(CBaseCombatCharacter_Event_Killed)(infooverride);
                return;
            }
        }
		DETOUR_MEMBER_CALL(CBaseCombatCharacter_Event_Killed)(info);
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_Spawn)();
		CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this); 
        auto mod = player->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            mod->FireCallback(ON_SPAWN);
        }

	}
    
    bool CheckDeploy(CallbackType type, CBaseEntity *player, CBaseEntity *entity)
    {
        auto mod = player->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            bool give = true;
            std::function<void(LuaState *)> func = [&](LuaState *state){
                LEntityAlloc(state->GetState(),entity);
            };
            std::function<void(LuaState *)> funcReturn = [&](LuaState *state){
                give = lua_isnil(state->GetState(), -1) || lua_toboolean(state->GetState(), -1);
            };
            mod->FireCallback(type, &func, 1, &funcReturn, 1);
            return give;
        }
        return true;
    }

    DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		auto entity = reinterpret_cast<CBaseCombatWeapon *>(this);
        if (!CheckDeploy(ON_EQUIP_ITEM, owner, entity)) return;
        
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_Equip)(owner);
	}

    DETOUR_DECL_MEMBER(void, CTFWearable_Equip, CBasePlayer *owner)
	{
		auto entity = reinterpret_cast<CTFWearable *>(this);
        if (!CheckDeploy(ON_EQUIP_ITEM, owner, entity)) return;
        
		DETOUR_MEMBER_CALL(CTFWearable_Equip)(owner);
	}

    DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Holster, CBaseCombatWeapon *newWeapon)
	{
		auto wep = reinterpret_cast<CTFWeaponBase *>(this);

        auto mod = wep->GetOwnerEntity() != nullptr ? wep->GetOwnerEntity()->GetEntityModule<LuaEntityModule>("luaentity") : nullptr;
        if (mod != nullptr) {
            bool give = true;
            std::function<void(LuaState *)> func = [&](LuaState *state){
                LEntityAlloc(state->GetState(),wep);
                LEntityAlloc(state->GetState(),newWeapon);
            };
            std::function<void(LuaState *)> funcReturn = [&](LuaState *state){
                give = lua_isnil(state->GetState(), -1) || lua_toboolean(state->GetState(), -1);
            };
            mod->FireCallback(ON_DEPLOY_WEAPON, &func, 2, &funcReturn, 1);
            if (!give) return false;
        }
		
		return DETOUR_MEMBER_CALL(CTFWeaponBase_Holster)(newWeapon);
	}

    class PlayerLoadoutUpdatedListener : public IBitBufUserMessageListener
	{
	public:

		CTFPlayer *player = nullptr;
		virtual void OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
		{
			if (pFilter->GetRecipientCount() > 0) {
				int id = pFilter->GetRecipientIndex(0);
				player = ToTFPlayer(UTIL_PlayerByIndex(id));
			}
		}
		virtual void OnPostUserMessage(int msg_id, bool sent)
		{
			if (sent && player != nullptr) {
                auto mod = player->GetEntityModule<LuaEntityModule>("luaentity");
                if (mod != nullptr) {
                    mod->FireCallback(ON_PROVIDE_ITEMS);
                }
			}
		}
	};

    bool DoCollideTest(CBaseEntity *entity1, CBaseEntity *entity2, bool &result) {
        auto mod1 = entity1->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod1 != nullptr && !mod1->callbacks[ON_SHOULD_COLLIDE].empty()) {
            auto &callbackList = mod1->callbacks[ON_SHOULD_COLLIDE];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity1);
                LEntityAlloc(l, entity2);
                mod1->CallCallback(callbackList, it, 2, 1);
                if (lua_type(l, -1) == LUA_TBOOLEAN) {
                    result = lua_toboolean(l, -1);
                    lua_settop(l, 0);
                    return true;
                }
                lua_settop(l, 0);
            }
        }
        auto mod2 = entity2->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod2 != nullptr && !mod2->callbacks[ON_SHOULD_COLLIDE].empty()) {
            auto &callbackList = mod2->callbacks[ON_SHOULD_COLLIDE];
            for (auto it = callbackList.begin(); it != callbackList.end();) {
                auto &callback = *it;
                if (callback.deleted) {it++; continue;}
                auto l = callback.state->GetState();
                lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                LEntityAlloc(l, entity2);
                LEntityAlloc(l, entity1);
                mod2->CallCallback(callbackList, it, 2, 1);
                if (lua_type(l, -1) == LUA_TBOOLEAN) {
                    result = lua_toboolean(l, -1);
                    lua_settop(l, 0);
                    return true;
                }
                lua_settop(l, 0);
            }
        }
        return false;
    }
    enum
    {
        // This bit will be set in GetRefEHandle for all static props
        STATICPROP_EHANDLE_MASK = 0x40000000
    };
    DETOUR_DECL_STATIC(bool, PassServerEntityFilter, IHandleEntity *ent1, IHandleEntity *ent2)
	{
        auto ret = DETOUR_STATIC_CALL(PassServerEntityFilter)(ent1, ent2);
        {
            if (ret) {
                auto entity1 = (CBaseEntity*) ent1;
                auto entity2 = (CBaseEntity*) ent2;
                
                bool result;
                if (entity1 != entity2 && entity1 != nullptr && entity2 != nullptr && DoCollideTest(entity1, entity2, result))
                {
                    return result;
                }
            }
        }
        return ret;
    }

    DETOUR_DECL_MEMBER(int, CCollisionEvent_ShouldCollide, IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1)
	{
        {
            CBaseEntity *entity1 = static_cast<CBaseEntity *>(pGameData0);
            CBaseEntity *entity2 = static_cast<CBaseEntity *>(pGameData1);

            bool result;
            if (entity1 != entity2 && entity1 != nullptr && entity2 != nullptr && DoCollideTest(entity1, entity2, result))
            {
                return result;
            }
        }
        return DETOUR_MEMBER_CALL(CCollisionEvent_ShouldCollide)(pObj0, pObj1, pGameData0, pGameData1);
    }

    bool DoTouchCallback(CallbackType type, CBaseEntity *entity, CBaseEntity *other)
    {
        auto mod = entity->GetEntityModule<LuaEntityModule>("luaentity");
        if (mod != nullptr) {
            std::function<void(LuaState *)> func = [&](LuaState *state){
                LEntityAlloc(state->GetState(), other);
                trace_t &tr = CBaseEntity::GetTouchTrace();
                auto pos = LVectorAlloc(state->GetState());
                *pos = tr.endpos;
                auto normal = LVectorAlloc(state->GetState());
                *normal = tr.plane.normal;
            };
            mod->FireCallback(type, &func, 3);
        }
        return true;
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_StartTouch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_START_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBaseEntity_StartTouch)(other);
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_EndTouch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_END_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBaseEntity_EndTouch)(other);
    }

    DETOUR_DECL_MEMBER(void, CBaseTrigger_StartTouch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_START_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBaseTrigger_StartTouch)(other);
    }

    DETOUR_DECL_MEMBER(void, CBaseTrigger_EndTouch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_END_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBaseTrigger_EndTouch)(other);
    }

    DETOUR_DECL_MEMBER(void, CBaseEntity_Touch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBaseEntity_Touch)(other);
    }

    DETOUR_DECL_MEMBER(void, CBasePlayer_Touch, CBaseEntity *other)
	{
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        if (!DoTouchCallback(ON_TOUCH, entity, other)) return; 
        
        return DETOUR_MEMBER_CALL(CBasePlayer_Touch)(other);
    }
    class CGameEvent : public IGameEvent
    {
    public:
        CGameEvent( void *descriptor );
        virtual ~CGameEvent();

        const char *GetName() const;
        bool  IsEmpty(const char *keyName = NULL);
        bool  IsLocal() const;
        bool  IsReliable() const;

        bool  GetBool( const char *keyName = NULL, bool defaultValue = false );
        int   GetInt( const char *keyName = NULL, int defaultValue = 0 );
        float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f );
        const char *GetString( const char *keyName = NULL, const char *defaultValue = "" );

        void SetBool( const char *keyName, bool value );
        void SetInt( const char *keyName, int value );
        void SetFloat( const char *keyName, float value );
        void SetString( const char *keyName, const char *value );
        
        void	*m_pDescriptor;
        KeyValues				*m_pDataKeys;
    };

    enum CallbackAction
    {
        ACTION_CONTINUE,
        ACTION_STOP,
        ACTION_MODIFY
    };

    DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr && !event_callbacks.empty()) {
            auto cevent = reinterpret_cast<CGameEvent *>(event);

            for (auto &callback : event_callbacks) {
                if (callback.name == event->GetName()) {
                    auto l = callback.state->GetState();
                    lua_newtable(l);
                    lua_rawgeti(l, LUA_REGISTRYINDEX, callback.func);
                    lua_pushvalue(l,-2);
                    FOR_EACH_SUBKEY(cevent->m_pDataKeys, subkey) {
                        lua_pushstring(l, subkey->GetString());
                        lua_setfield(l, -2, subkey->GetName());
                    }
                    callback.state->Call(1,1);
                    int result = luaL_optinteger(l, -1, ACTION_CONTINUE);
                    if (result == ACTION_MODIFY) {
                        lua_pop(l,1);
                        lua_pushnil(l);
                        while (lua_next(l, -2)) {
                            lua_pushvalue(l, -2);
                            event->SetString(lua_tostring(l, -1), lua_tostring(l, -2));
                            lua_pop(l, 2);
                        }
                    }
                    if (result == ACTION_STOP) {
                        lua_pop(l,1);
			            mgr->FreeEvent(event);
                        return false;
                    }
                    lua_pop(l,1);
                }
            }
        } 
		
		return DETOUR_MEMBER_CALL(IGameEventManager2_FireEvent)(event, bDontBroadcast);
	}

    class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:Lua") 
        {
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntity_AcceptInput, "CBaseEntity::AcceptInput", HIGH);
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntityOutput_FireOutput, "CBaseEntityOutput::FireOutput", HIGH);
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage", HIGH);
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseCombatCharacter_OnTakeDamage, "CBaseCombatCharacter::OnTakeDamage", LOWEST);
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_Activate, "CBaseEntity::Activate");
            MOD_ADD_DETOUR_STATIC(DispatchSpawn, "DispatchSpawn");
            MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
            MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateOnRemove, "CTFPlayer::UpdateOnRemove");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_PlayerRunCommand, "CTFPlayer::PlayerRunCommand");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_Event_Killed, "CBaseEntity::Event_Killed");
            MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_Event_Killed, "CBaseCombatCharacter::Event_Killed");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Spawn, "CTFPlayer::Spawn");
            MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_Equip, "CBaseCombatWeapon::Equip");
            MOD_ADD_DETOUR_MEMBER(CTFWearable_Equip, "CTFWearable::Equip");
            MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Holster, "CTFWeaponBase::Holster");
            MOD_ADD_DETOUR_STATIC(PassServerEntityFilter, "PassServerEntityFilter");
            MOD_ADD_DETOUR_MEMBER(CCollisionEvent_ShouldCollide, "CCollisionEvent::ShouldCollide");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_StartTouch, "CBaseEntity::StartTouch");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_EndTouch, "CBaseEntity::EndTouch");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_Touch, "CBaseEntity::Touch");
            MOD_ADD_DETOUR_MEMBER(CBaseTrigger_StartTouch, "CBaseTrigger::StartTouch");
            MOD_ADD_DETOUR_MEMBER(CBaseTrigger_EndTouch, "CBaseTrigger::EndTouch");
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_Touch, "CBasePlayer::Touch");
            MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");
            
        }
		
        virtual void PreLoad() override
        {
            stringSendProxy = AddrManager::GetAddr("SendProxy_StringToString");
        }

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
	    virtual bool EnableByDefault() { return true; }

        virtual void OnEnable() override
		{
            sendproxies = gamedll->GetStandardSendProxies();
            
			usermsgs->HookUserMessage2(usermsgs->GetMessageIndex("PlayerLoadoutUpdated"), &player_loadout_updated_listener);
        }

		virtual void OnDisable() override
		{
			usermsgs->UnhookUserMessage2(usermsgs->GetMessageIndex("PlayerLoadoutUpdated"), &player_loadout_updated_listener);
		}

        virtual void LevelInitPreEntity() override
        { 
            if (state != nullptr) { delete state; state = nullptr;}

            sendproxies = gamedll->GetStandardSendProxies();
        }

		virtual void FrameUpdatePostEntityThink() override
		{
            static double script_exec_time_tick_max = 0.0;
            if (script_exec_time_tick > script_exec_time_tick_max ) {
                script_exec_time_tick_max = script_exec_time_tick;
            }
            script_exec_time_tick = 0.0;
            if (gpGlobals->tickcount % 66 == 0) {
                for (int i = 0; i < 34; i++) {
                    if (profile[i]) {
                        auto player = UTIL_PlayerByIndex(i);
                        if (player != nullptr) {
                            ClientMsg(player, "Lua script execution time: [avg: %.9fs (%d%%)| max: %.9fs (%d%%)]\n", script_exec_time / 66, (int)(script_exec_time * 100), script_exec_time_tick_max, (int)((script_exec_time_tick_max / 0.015) * 100) );
                        }
                    }
                }
                script_exec_time = 0;
                script_exec_time_tick_max = 0;
            }

            if (state != nullptr)
			    state->UpdateTimers();

            if (!LuaState::List().empty())
            {
                for(auto state : LuaState::List()) {
                    
                    if (state->CheckGlobal("OnGameTick")) {
                        state->Call(0, 0);
                    }
                }
            }
		}
        
		PlayerLoadoutUpdatedListener player_loadout_updated_listener;
	};
	CMod s_Mod;
}