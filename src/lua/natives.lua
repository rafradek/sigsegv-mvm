---@class Vector
---@field x number
---@field y number
---@field z number
local vector = {}

-- Creates a vector with set x, y, z coordinates
---@param x number
---@param y number
---@param z number
---@return Vector
function Vector(x, y, z) end

---@return Vector
function vector:Normalize() end

---@return number
function vector:Length() end

---@param other Vector
---@return number
function vector:Distance(other) end

---@param other Vector
---@return number
function vector:Dot(other) end

---@param other Vector
---@return Vector
function vector:Cross(other) end

---@param angle Vector
---@return Vector
function vector:Rotate(angle) end

---@return Vector
function vector:ToAngles() end

---@return Vector
function vector:GetForward() end

---@return Vector, Vector, Vector
function vector:GetAngleVectors() end

---@param vector Vector
---@return nil
function vector:Copy(vector) end

---@param x number
---@param y number
---@param z number
---@return nil
function vector:CopyUnpacked(x, y, z) end

---@class Entity
local entity = {}

--Creates an entity with specified classname
--Alternatively, if classname is a number, returns an entity with handle/network index if it exists
---@param classname string
---@param spawn? boolean = true. Spawn entity after creation
---@param activate? boolean = true. Activate entity after creation
---@overload fun(classname: number, spawn: nil, activate: nil)
---@return Entity
function Entity(classname, spawn, activate) end

---@return boolean
function entity:IsValid() end

---@return number handleId
function entity:GetHandleIndex() end

---@return number networkId
function entity:GetNetworkIndex() end

---@return nil
function entity:Spawn() end

---@return nil
function entity:Activate() end

---@return nil
function entity:Remove() end

-- Returns targetname of the entity
---@return string
function entity:GetName() end

-- Set targetname of the entity
---@param name string
---@return nil
function entity:SetName(name) end

---@return string
function entity:GetPlayerName() end

---@return boolean
function entity:IsAlive() end

---@return boolean # `true` if the entity is a player (real or bot)
function entity:IsPlayer() end

---@return boolean # `true` if the entity has AI but is not a player bot. Engineer buildings are not NPC
function entity:IsNPC() end

---@return boolean # `true` if the entity is a player bot, `false` otherwise. Returns `false` if entity is SourceTV bot
function entity:IsBot() end

---@return boolean # `true` if the entity is a real player, and not a bot, `false` otherwise
function entity:IsRealPlayer() end

---@return boolean
function entity:IsWeapon() end

---@return boolean # `true` if the entity is an Engineer building or sapper, `false` otherwise
function entity:IsObject() end

---@return boolean # `true` if the entity is an NPC, player or a building, `false` otherwise
function entity:IsCombatCharacter() end

---@return boolean # `true` if the entity is a player cosmetic item, `false` otherwise
function entity:IsWearable() end

---@return string
function entity:GetClassname() end

--Adds callback function for a specific action, check ON_* globals for more info
---@param type number Action to use, check ON_* globals
---@param func function Callback function. Function parameters depend on the callback type
---@return number id Can be used to remove a callback with `RemoveCallback` function
function entity:AddCallback(type, func) end

--Removes callback added with `AddCallback` function
---@param type number Action to use, check ON_* globals
---@param id number Callback id
---@return nil
function entity:RemoveCallback(type, id) end

--Fires an entity input
---@param name string Name of the input
---@param value? any Value passed to the input
---@param activator? Entity The activator entity
---@param caller? Entity The caller entity
---@return boolean `true` if the input exists and was called successfully, `false` otherwise
function entity:AcceptInput(name, value, activator, caller) end

--Returns player item in a slot number
---@param slot number Slot number, check LOADOUT_POSITION_* globals
---@return Entity 
---@return nil #No item found in the specified slot
function entity:GetPlayerItemBySlot(slot) end

--Returns player item by name
---@param name string Item definition name
---@return Entity
---@return nil #No item found with the specified name
function entity:GetPlayerItemByName(name) end

--Returns item or player attribute value
---@param name string Attribute definition name
---@return string|number|nil value Value of the attribute
function entity:GetAttributeValue(name) end

--Sets item or player attribute value. Value of nil removes the attribute
---@param name string Attribute definition name
---@param value string|number|nil Attribute value
---@return nil
function entity:SetAttributeValue(name, value) end

--Returns a table of all properties (datamap, sendprop, custom) as keys and their values.
--The table is read only, changes must be written to the entity variable itself
---@return table
function entity:DumpProperties() end

--Returns a table containing all inputs of the entity.
--The inputs can be called directly as functions. Example: `ent:FireUser1(value,activator,caller)`
---@return table
function entity:DumpInputs() end

---@class TakeDamageInfo
---@field Attacker Entity|nil
---@field Inflictor Entity|nil
---@field Weapon Entity|nil
---@field Damage number
---@field DamageType number
---@field DamageCustom number
---@field CritType number

--Deal damage to the entity
---@param damageInfo TakeDamageInfo
---@return number damageDealt Damage dealt to the entity
function entity:TakeDamage(damageInfo) end

--Add condition to player. Check TF_COND_* globals for the list of conditions
---@param condition number
---@param duration? number #Optional duration in seconds
---@param provider? Entity #Optional player that caused the condition
---@return nil
function entity:AddCond(condition, duration, provider) end

--Remove condition from player. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return nil
function entity:RemoveCond(condition) end

--Check if player has the condition applied. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return boolean
function entity:InCond(condition) end

--Get player that provided the condition. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return boolean
function entity:GetConditionProvider(condition) end

--Stun a player, slowing him down and/or making him unable to attack. Check TF_STUNFLAG_* globals
---@param duration number How long should the stun last in seconds
---@param amount number Movement speed penalty when TF_STUNFLAG_SLOWDOWN flag is set. The number should be between 0 and 1. 0 - 450 speed limit, 1 - no movement
---@param flags number Stun flags to set
---@param stunner? Entity Optional player that caused the stun
---@return boolean
function entity:StunPlayer(duration, amount, flags, stunner) end

---@return Vector
function entity:GetAbsOrigin() end

---@param vec Vector
---@return nil
function entity:SetAbsOrigin(vec) end

---@return Vector
function entity:GetAbsAngles() end

---@param angles Vector
---@return nil
function entity:SetAbsAngles(angles) end

--Teleports entity to a location, optionally also sets angles and velocity
---@param pos Vector|nil
---@param angles? Vector|nil
---@param velocity? Vector|nil
---@return nil
function entity:Teleport(pos, angles, velocity) end

--Creates an item. The item will be given to the player
---@param name string
---@param attrs? table Optional table with attribute key value pairs applied to the item
---@param noRemove? boolean = false. Do not remove previous item in the slot.
---@param forceGive? boolean = true. Forcibly give an item even if the player class does not match.
---@return Entity|nil item The created item or nil on failure
function entity:GiveItem(name, attrs, noRemove, forceGive) end

--Fire entity output by name
---@param name string
---@param value? any
---@param activator? Entity
---@param delay? number
---@return nil
function entity:FireOutput(name, value, activator, delay) end

--Set fake send prop value. This value is only seen by clients, not the server
---@param name string
---@param value any
---@return nil
function entity:SetFakeSendProp(name, value) end

--Reset fake send prop value. This value is only seen by clients, not the server
---@param name string
---@return nil
function entity:ResetFakeSendProp(name) end

--Get fake send prop value. This value is only seen by clients, not the server
---@param name string
---@return any
function entity:GetFakeSendProp(name) end

--Add effects to an entity. Check EF_* globals
---@param effect number
---@return nil
function entity:AddEffects(effect) end

--Remove effects from an entity. Check EF_* globals
---@param effect number
---@return nil
function entity:RemoveEffects(effect) end

--Returns if effect is active. Check EF_* globals
---@param effect number
---@return boolean
function entity:IsEffectActive(effect) end

ents = {}

--Finds first matching entity by targetname. Trailing wildcards and @ selectors apply
---@param name string
---@param prev ?Entity #Find next matching entity after this entity
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByName(name, prev) end

--Finds first matching entity by classname. Trailing wildcards and @ selectors apply
---@param classname string
---@param prev ?Entity #Find next matching entity after this entity
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByClassname(classname, prev) end

--Finds all matching entities by name. Trailing wildcards and @ selectors apply
---@param name string
---@return table entities All entities that matched the criteria
function ents.FindAllByName(name) end

--Finds all matching entities by classname. Trailing wildcards and @ selectors apply
---@param classname string
---@return table entities All entities that matched the criteria
function ents.FindAllByClassname(classname) end

--Finds all entities in a box
---@param mins Vector Starting box coordinates
---@param maxs Vector Ending box coordinates
---@return table entities All entities that matched the criteria
function ents.FindAllInBox(mins, maxs) end

--Finds all entities in a sphere
---@param center Vector Sphere center coordinates
---@param radius number Sphere radius
---@return table entities All entities that matched the criteria
function ents.FindAllInSphere(center, radius) end

--Returns first entity
---@return Entity first First entity in the list
function ents.FirstEntity() end

--Returns next entity after previous entity
---@param prev Entity The previous entity
---@return Entity first The next entity in the list
function ents.NextEntity(prev) end

timer = {}

--Creates a simple timer that calls the function after delay
---@param delay number Delay in seconds before firing the function
---@param func function Function to call 
---@return number id #id that can be used to stop the timer with `timer.Stop`
function timer.Simple(delay, func) end

--Creates a timer that calls the function after delay, with repeat count, and paramater
---@param delay number Delay in seconds before firing the function
---@param func function Function to call. If param is set, calls the function with a sigle provided value
---@param repeats? number = 1. Number of timer repeats. 0 = Infinite
---@param param? any Parameter to pass to the function
---@return number id #id that can be used to stop the timer with `timer.Stop`
function timer.Create(delay, func, repeats, param) end

--Stops the timer with specified id
---@param id number id of the timer
---@return nil
function timer.Stop(id) end

util = {}

---@class TraceInfo
---@field start Vector|Entity
---@field endpos Entity
---@field distance number
---@field angles Vector
---@field mask number
---@field collisiongroup number
---@field mins Vector
---@field maxs Vector
---@field filter function|table|Entity

---@class TraceResultInfo
---@field Entity Entity
---@field Fraction number
---@field FractionLeftSolid number
---@field Hit boolean
---@field HitBox number
---@field HitGroup number
---@field HitNoDraw boolean
---@field HitNonWorld boolean
---@field HitNormal Vector
---@field HitPos Vector
---@field HitSky boolean
---@field HitTexture string
---@field HitWorld boolean
---@field Normal Vector
---@field StartPos Vector
---@field StartSolid boolean
---@field SurfaceFlags number
---@field DispFlags number
---@field Contents number
---@field SurfaceProps number
---@field PhysicsBone number

--Fires a trace
---@param trace TraceInfo trace table to use 
---@return TraceResultInfo #trace result table
function util.Trace(trace) end

--Prints message to player's console
---@param player Entity
---@vararg any
---@return nil
function util.PrintToConsole(player, ...) end

--Prints console message to all players
---@vararg any
---@return nil
function util.PrintToConsoleAll(...) end

--Prints message to player's chat
---@param player Entity
---@vararg any
---@return nil
function util.PrintToChat(player, ...) end

--Prints chat message to all players
---@vararg any
---@return nil
function util.PrintToChatAll(...) end

--Returns time in seconds since map load
---@return number
function CurTime() end

--Returns tick coount since map load
---@return number
function TickCount() end

--Returns current map name
---@return string
function GetMapName() end

--Checks if the passed value is not nil and a valid Entity
---@param value any
---@return boolean
function IsValid(value) end