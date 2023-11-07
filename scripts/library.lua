---@class Vector
---@field x number
---@field y number
---@field z number
local CVector = {}

-- Creates a vector with set x, y, z coordinates
---@param x number
---@param y number
---@param z number
---@return Vector
function Vector(x, y, z) end

---@return Vector
function CVector:Normalize() end

---@return number
function CVector:Length() end

---@param other Vector
---@return number
function CVector:Distance(other) end

---@param other Vector
---@return number
function CVector:Dot(other) end

---@param other Vector
---@return Vector
function CVector:Cross(other) end

---@param angle Vector
---@return Vector
function CVector:Rotate(angle) end

---@return Vector
function CVector:ToAngles() end

---@return Vector
function CVector:GetForward() end

---@return Vector, Vector, Vector
function CVector:GetAngleVectors() end

---@param vector Vector
---@return nil
function CVector:Copy(vector) end

---@param x number
---@param y number
---@param z number
---@return nil
function CVector:CopyUnpacked(x, y, z) end

--Game entity class.
--It is possible to access entity netprops, datamap and custom variables like a typical field, like entity.m_iTeamNum or entity.m_iAmmo[1] (Arrays start from 1 instead of 0). You should omit $ from custom prop names
--List of netprops https://raw.githubusercontent.com/powerlord/tf2-data/master/netprops.txt
--List of datamap props https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
--Is is also possible to call entity inputs like a typical function, like entity:FireUser1(). You should omit $ from custom input names
---@class Entity
local CEntity = {}

--Creates an entity with specified classname
--Alternatively, if classname is a number, returns an entity with handle/network index if it exists
---@param classname string
---@param spawn? boolean `= true`. Spawn entity after creation
---@param activate? boolean `= true`. Activate entity after creation
---@overload fun(classname: number, spawn: nil, activate: nil)
---@return Entity
function Entity(classname, spawn, activate) end

--Checks if the passed value is not nil and a valid Entity
---@param value any
---@return boolean
function IsValid(value) end

---@return boolean
function CEntity:IsValid() end

---@return number handleId
function CEntity:GetHandleIndex() end

---@return number networkId
function CEntity:GetNetIndex() end

---@return nil
function CEntity:DispatchSpawn() end

---@return nil
function CEntity:Activate() end

---@return nil
function CEntity:Remove() end

-- Returns targetname of the entity
---@return string
function CEntity:GetName() end

-- Set targetname of the entity
---@param name string
---@return nil
function CEntity:SetName(name) end

---@return string
function CEntity:GetPlayerName() end

---@return boolean
function CEntity:IsAlive() end

---@return boolean # `true` if the entity is a player (real or bot)
function CEntity:IsPlayer() end

---@return boolean # `true` if the entity has AI but is not a player bot. Engineer buildings are not NPC
function CEntity:IsNPC() end

---@return boolean # `true` if the entity is a player bot, `false` otherwise. Returns `false` if entity is SourceTV bot
function CEntity:IsBot() end

---@return boolean # `true` if the entity is a real player, and not a bot, `false` otherwise
function CEntity:IsRealPlayer() end

---@return boolean
function CEntity:IsWeapon() end

---@return boolean # `true` if the entity is an Engineer building or sapper, `false` otherwise
function CEntity:IsObject() end

---@return boolean # `true` if the entity is an NPC, player or a building, `false` otherwise
function CEntity:IsCombatCharacter() end

---@return boolean # `true` if the entity is a player cosmetic item, `false` otherwise
function CEntity:IsWearable() end

---@return boolean # `true` if the entity is a weapon or cosmetic, `false` otherwise
function CEntity:IsItem() end

---@return boolean # `true` if the entity is the world entity, `false` otherwise
function CEntity:IsWorld() end

---@return string
function CEntity:GetClassname() end

--Adds callback function for a specific action, check ON_* globals for more info
---@param type number Action to use, check ON_* globals
---@param func function Callback function. Function parameters depend on the callback type
---@return number id Can be used to remove a callback with `RemoveCallback` function
function CEntity:AddCallback(type, func) end

--Removes callback added with `AddCallback` function
---@param type? number Optional action type to use, check ON_* globals. Not really needed
---@param id number Callback id
---@return nil
function CEntity:RemoveCallback(type, id) end

--Removes all callbacks added with `AddCallback` function
---@param type? number Optionally only delete callbacks of specified type, check ON_* globals
---@return nil
function CEntity:RemoveAllCallbacks(type) end

--Fires an entity input
---@param name string Name of the input
---@param value? any Value passed to the input
---@param activator? Entity The activator entity
---@param caller? Entity The caller entity
---@return boolean `true` if the input exists and was called successfully, `false` otherwise
function CEntity:AcceptInput(name, value, activator, caller) end

--Returns player item in a slot number
---@param slot number Slot number, check LOADOUT_POSITION_* globals
---@return Entity 
---@return nil #No item found in the specified slot
function CEntity:GetPlayerItemBySlot(slot) end

--Returns player item by name
---@param name string Item definition name
---@return Entity
---@return nil #No item found with the specified name
function CEntity:GetPlayerItemByName(name) end

--Returns item or player attribute value. Ignores item definition attributes unless specified
---@param name string|number Attribute definition name or index
---@param checkDefinition? boolean `= false`. Enables checking for attributes in the item definition as well
---@return string|number|nil value Value of the attribute
function CEntity:GetAttributeValue(name, checkDefinition) end

--Returns item or player attribute value by class (like "mult_dmg"), combines all attributes with matching class, including attributes from the player owner.
---@param name string Attribute class name
---@param default number Default value to use. Typically 1 for multiply attributes, 0 for additive attributes
---@return number value Value of an attribute class, default value if not found
function CEntity:GetAttributeValueByClass(name, default) end

--Returns all item or player attribute values. Ignores item definition attributes unless specified
---@param checkDefinition? boolean `= false`. Enables checking for attributes in the item definition as well
---@return table values Table containing attribute name -> value keys
function CEntity:GetAllAttributeValues(checkDefinition) end

--Returns item name
---@return string name Item name in item definition
function CEntity:GetItemName() end

--Returns item name displayed in game
---@return string name Item name displayed in game
function CEntity:GetItemNameForDisplay() end

--Returns a table containing all player items
---@return table items Table containing all items
function CEntity:GetAllItems() end

--Sets item or player attribute value. Value of nil removes the attribute
---@param name string|number Attribute definition name or index
---@param value string|number|nil Attribute value
---@return nil
function CEntity:SetAttributeValue(name, value) end

--Returns a table of all properties (datamap, sendprop, custom) as keys and their values.
--Only use this function for debugging, it is slow compared to directly getting property value from entity
--The table is read only, changes must be written to the entity itself, like entity.m_iTeamNum = 3
---@return table
function CEntity:DumpProperties() end

--Returns a table containing all inputs of the entity.
--Use this function for debugging
--The inputs can be called directly as functions. Example: `ent:FireUser1(value,activator,caller)`
---@return table
function CEntity:DumpInputs() end

--Deal damage to the entity
---@param damageInfo TakeDamageInfo See DefaultTakeDamageInfo
---@return number damageDealt Damage dealt to the entity
function CEntity:TakeDamage(damageInfo) end

--Add health to the entity
---@param amount number
---@param overheal? boolean
---@return number damageDealt Damage dealt to the entity
function CEntity:AddHealth(amount, overheal) end

--Add condition to player. Check TF_COND_* globals for the list of conditions
---@param condition number
---@param duration? number #Optional duration in seconds
---@param provider? Entity #Optional player that caused the condition
---@return nil
function CEntity:AddCond(condition, duration, provider) end

--Remove condition from player. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return nil
function CEntity:RemoveCond(condition) end

--Check if player has the condition applied. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return boolean
function CEntity:InCond(condition) end

--Get player that provided the condition. Check TF_COND_* globals for the list of conditions
---@param condition number
---@return Entity
function CEntity:GetConditionProvider(condition) end

--Stun a player, slowing him down and/or making him unable to attack. Check TF_STUNFLAG_* globals
---@param duration number How long should the stun last in seconds
---@param amount number Movement speed penalty when TF_STUNFLAG_SLOWDOWN flag is set. The number should be between 0 and 1. 0 - 450 speed limit, 1 - no movement
---@param flags number Stun flags to set. Flags can be combined with | operator. Check TF_STUNFLAG_* globals
---@param stunner? Entity Optional player that caused the stun
---@return boolean
function CEntity:StunPlayer(duration, amount, flags, stunner) end

---@return Vector
function CEntity:GetAbsOrigin() end

---@param vec Vector
---@return nil
function CEntity:SetAbsOrigin(vec) end

---@return Vector
function CEntity:GetAbsAngles() end

---@param angles Vector
---@return nil
function CEntity:SetAbsAngles(angles) end

--Teleports entity to a location, optionally also sets angles and velocity
---@param pos Vector|nil
---@param angles? Vector|nil
---@param velocity? Vector|nil
---@return nil
function CEntity:Teleport(pos, angles, velocity) end

--Creates an item. The item will be given to the player
---@param name string
---@param attrs? table Optional table with attribute key value pairs applied to the item
---@param noRemove? boolean = false. Do not remove previous item in the slot.
---@param forceGive? boolean = true. Forcibly give an item even if the player class does not match.
---@return Entity|nil item The created item or nil on failure
function CEntity:GiveItem(name, attrs, noRemove, forceGive) end

--Fire entity output by name
---@param name string
---@param value? any
---@param activator? Entity
---@param delay? number
---@return nil
function CEntity:FireOutput(name, value, activator, delay) end

--Set fake send prop value. This value is only seen by clients, not the server
---@param name string
---@param value any
---@return nil
function CEntity:SetFakeSendProp(name, value) end

--Reset fake send prop value. This value is only seen by clients, not the server
---@param name string
---@return nil
function CEntity:ResetFakeSendProp(name) end

--Get fake send prop value. This value is only seen by clients, not the server
---@param name string
---@return any
function CEntity:GetFakeSendProp(name) end

--Add effects to an entity. Check EF_* globals
---@param effect number
---@return nil
function CEntity:AddEffects(effect) end

--Remove effects from an entity. Check EF_* globals
---@param effect number
---@return nil
function CEntity:RemoveEffects(effect) end

--Returns if effect is active. Check EF_* globals
---@param effect number
---@return boolean
function CEntity:IsEffectActive(effect) end

--Prints message to player.
---@param printTarget number See PRINT_TARGET_* globals
---@vararg any
---@return nil
function CEntity:Print(printTarget, ...) end

--Prints message to player
---@param params ShowHUDTextParams Table containing params. See DefaultHudTextParams global
---@vararg any
---@return nil
function CEntity:ShowHudText(params, ...) end

--Displays menu to player
---@param menu Menu See DefaultMenu globals
---@return nil
function CEntity:DisplayMenu(menu) end

--Hide current menu from the player
---@return nil
function CEntity:HideMenu() end

--Snap player view to specified angle
---@param angle Vector angle to snap to
---@return nil
function CEntity:SnapEyeAngles(angle) end

--Get player user id
---@return number userid User id of of the player 
function CEntity:GetUserId() end

--Get player steam id
---@return number steamid Steam id of of the player 
function CEntity:GetSteamId() end

--Get client ConVar value
---@param name string Name of the ConVar
---@return string value Value of the client ConVar
function CEntity:GetClientConVar(name) end

--Set client ConVar value for bots
---@param name string Name of the ConVar
---@param value string Value of the ConVar
---@return nil
function CEntity:SetFakeClientConVar(name, value) end


----------------
-- Entity inputs
----------------

function CEntity:FireUser1(_,activator, caller) end
function CEntity:FireUser2(_,activator, caller) end
function CEntity:FireUser3(_,activator, caller) end
function CEntity:FireUser4(_,activator, caller) end

function CEntity:FireUser5(value, activator, caller) end
function CEntity:FireUser6(value, activator, caller) end
function CEntity:FireUser7(value, activator, caller) end
function CEntity:FireUser8(value, activator, caller) end

function CEntity:FireUserAsActivator1(value, _, caller) end
function CEntity:FireUserAsActivator2(value, _, caller) end
function CEntity:FireUserAsActivator3(value, _, caller) end
function CEntity:FireUserAsActivator4(value, _, caller) end

function CEntity:SetModelOverride(value) end
function CEntity:SetModel(value) end
function CEntity:SetModelSpecial(value) end

function CEntity:MoveType(value) end
function CEntity:PlaySound(value) end
function CEntity:StopSound(value) end

function CEntity:SetLocalOrigin(value) end
function CEntity:SetLocalAngles(value) end
function CEntity:SetLocalVelocity(value) end
function CEntity:SetForwardVelocity(value) end

function CEntity:AddOutput(value) end
function CEntity:RemoveOutput(value) end
function CEntity:CancelPending() end

function CEntity:SetCollisionFilter(value) end
function CEntity:ClearFakeParent() end
function CEntity:SetFakeParent(entity) end
function CEntity:SetAimFollow(entity) end
function CEntity:FaceEntity(entity) end
function CEntity:RotateTowards(entity) end

function CEntity:AddModule(name) end
function CEntity:RemoveModule(name) end

--Hides entity to the provided player
---@param player Entity
---@return nil
function CEntity:HideTo(player) end

--Shows entity to the provided player, if it was previously hidden
---@param player Entity
---@return nil
function CEntity:ShowTo(player) end

--Hides entity to all players by default
---@return nil
function CEntity:HideToAll() end

--Shows entity to all players, if it was previously hidden
---@return nil
function CEntity:ShowToAll() end

function CEntity:AddModule(name) end
function CEntity:StopParticleEffects() end
function CEntity:SetSolidFlags(flags) end
function CEntity:SetSolid(solidType) end

-------------
-- Player inputs
-------------

function CEntity:SwitchClass(name) end
function CEntity:SwitchClassInPlace(name) end
function CEntity:ForceRespawn() end
function CEntity:ForceRespawnDead() end
function CEntity:Suicide() end
function CEntity:ChangeAttributes(name) end
function CEntity:RollCommonSpell() end
function CEntity:RollRareSpell() end
function CEntity:SetSpell(nameOrIndex) end
function CEntity:AddSpell(nameOrIndex) end
function CEntity:PlaySoundToSelf(name) end
function CEntity:IgnitePlayerDuration(duration, igniter) end
function CEntity:WeaponSwitchSlot(slot) end
function CEntity:WeaponStripSlot(slot) end
function CEntity:RemoveItem(name) end
---@param slot? number Optional slot number
function CEntity:DropItem(slot) end
function CEntity:SetCurrency(currency) end
function CEntity:AddCurrency(currency) end
function CEntity:RemoveCurrency(currency) end
function CEntity:RefillAmmo() end
function CEntity:Regenerate() end
function CEntity:BotCommand(command) end
function CEntity:ResetInventory() end
function CEntity:PlaySequence(name) end
function CEntity:AwardExtraItem(name) end
function CEntity:BleedPlayer(duration) end
-- For player only
function CEntity:SetCustomModel(name) end
function CEntity:SetHUDVisibility(visible) end

ents = {}

--Finds matching entity by targetname. Trailing wildcards and @ selectors apply
---@param name string
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByName(name) end

--Finds first matching entity by targetname. Trailing wildcards and @ selectors apply
---@param name string
---@param prev ?Entity #Find next matching entity after this entity
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByNameAfter(name, prev) end

--Finds matching entity by classname. Trailing wildcards and @ selectors apply
---@param classname string
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByClass(classname) end

--Finds first matching entity by classname. Trailing wildcards and @ selectors apply
---@param classname string
---@param prev ?Entity #Find next matching entity after this entity
---@return Entity|nil #Entity if found, nil otherwise
function ents.FindByClassAfter(classname, prev) end

--Finds all matching entities by name. Trailing wildcards and @ selectors apply
---@param name string
---@return table entities All entities that matched the criteria
function ents.FindAllByName(name) end

--Finds all matching entities by classname. Trailing wildcards and @ selectors apply
---@param classname string
---@return table entities All entities that matched the criteria
function ents.FindAllByClass(classname) end

--Finds all entities in a box
---@param mins Vector Starting box coordinates
---@param maxs Vector Ending box coordinates
---@return table entities All entities that matched the criteria
function ents.FindInBox(mins, maxs) end

--Finds all entities in a sphere
---@param center Vector Sphere center coordinates
---@param radius number Sphere radius
---@return table entities All entities that matched the criteria
function ents.FindInSphere(center, radius) end

--Returns first entity
---@return Entity first First entity in the list
function ents.GetFirstEntity() end

--Returns next entity after previous entity
---@param prev Entity The previous entity
---@return Entity first The next entity in the list
function ents.GetNextEntity(prev) end

--Returns a table containing all entities
---@return table entities all entities
function ents.GetAll() end

--Returns a table containing all players
---@return table players all players
function ents.GetAllPlayers() end

--Adds an entity creation callback
---@param classname string Entity classname to which the callback will listen to. Supports trailing wildcards
---@param callback function Callback function, called with two parameters: entity, classname
---@return number id id of the callback for removal with `ents.RemoveCreateCallback(id)` 
function ents.AddCreateCallback(classname, callback) end

--Removes an entity creation callback
---@param id number Callback id
---@return nil
function ents.RemoveCreateCallback(id) end

--Finds a player with given userid, or nil if not found
---@param userid number
---@return Entity player Player with given user id, or `nil` if not found
function ents.GetPlayerByUserId(userid) end

--Creates an entity with specified classname and keyvalues
---@param classname string
---@param keyTable table key -> value table. Example: { origin = "0 0 0" }
---@param spawn? boolean = true. Spawn entity after creation
---@param activate? boolean = true. Activate entity after spawn
---@return Entity entity Entity that was created or `nil` in case of failure
function ents.CreateWithKeys(classname, keyTable, spawn, activate) end

--Spawns a point template from population file
---@param template string Template name
---@param templateInfo SpawnTemplateInfo Table containing template spawn paramaters. See DefaultSpawnTemplateInfo global
---@return table entities Table containing spawned entities. `nil` if template failed to spawn
function ents.SpawnTemplate(template, templateInfo) end


timer = {}

--Creates a simple timer that calls the function after delay
---@param delay number Delay in seconds before firing the function
---@param func function Function to call 
---@return number id #id that can be used to stop the timer with `timer.Stop`
function timer.Simple(delay, func) end

--Creates a timer that calls the function after delay, with repeat count, and paramater
---@param delay number Delay in seconds before firing the function
---@param func function Function to call. If param is set, calls the function with a sigle provided value. Return false to stop the repeaiting timer
---@param repeats? number = 1. Number of timer repeats. 0 = Infinite
---@param param? any Parameter to pass to the function
---@return number id #id that can be used to stop the timer with `timer.Stop`
function timer.Create(delay, func, repeats, param) end

--Stops the timer with specified id
---@param id number id of the timer
---@return nil
function timer.Stop(id) end


util = {}

--Fires a trace
---@param trace TraceInfo trace table to use. See DefaultTraceInfo
---@return TraceResultInfo #trace result table. See DefaultTraceResultInfo
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

--Displays a particle effect
---@param name string Name of the particle
---@param position Vector|nil Position of the particle
---@param angle Vector|nil Angle of the particle
---@param entity Entity|nil Entity to attach to
---@return nil
function util.ParticleEffect(name, position, angle, entity) end

--Returns name of item definition with specified id
---@param id number item definition index
---@return string|nil name Name of the item with specified id or nil if not found
function util.GetItemDefinitionNameByIndex(id) end

--Returns id of item definition with specified name
---@param name number item definition name
---@return number|nil id Id of the item with specified name or nil if not found
function util.GetItemDefinitionIndexByName(name) end

--Returns name of attribute definition with specified id
---@param id number attribute definition index
---@return string|nil name Name of the attribute with specified id or nil if not found
function util.GetAttributeDefinitionNameByIndex(id) end

--Returns id of attribute definition with specified name
---@param name number attribute definition name
---@return number|nil id Id of the attribute with specified name or nil if not found
function util.GetAttributeDefinitionIndexByName(name) end

--Check if lag compensation is active. Lag compensation allows traces fired from player eyes to be more accurate with what the player can see
---@return boolean
function util.IsLagCompensationActive() end

--Starts lag compensation. Useful when firing traces from player eyes. Make sure IsLagCompensationActive() returns false before calling this function, and FinishLagCompensation() is called right after firing the trace
---@param player Entity The player which should have their lag compensated
---@return nil
function util.StartLagCompensation(player) end

--Finishes lag compensation. Useful when firing traces from player eyes. Make sure StartLagCompensation() was called before calling this function
---@param player Entity The player which should have their lag no longer compensated
---@return nil
function util.FinishLagCompensation(player) end


tempents = {}

--Sends temporary entity such as explosion or particle. See https://raw.githubusercontent.com/powerlord/tf2-data/master/teprops.txt for a list of available entities
---@param name string Name of the temporary entity
---@param props table Table that contains property names and their values
---@param recipients? Entity|table|nil Recipient(s) of the temporary entity. Can be a single player, a table of players, or nil to send to all players
---@return nil
function tempents.Send(name, props, recipients) end

--Adds temporary entity callback. See https://raw.githubusercontent.com/powerlord/tf2-data/master/teprops.txt for a list of all available entities
---@param name string Name of the temporary entity
---@param callback function Callback function with parameters: propTable (contains entity property values). Return ACTION_MODIFY to send entity to clients with modified values, ACTION_STOP to stop the entity from being send to clients.
---@return number id id for later removal with `RemoveCallback(id)` function
function tempents.AddCallback(name, callback) end

--Removes temporary entity callback
---@param id number id of the entity callback
---@return nil
function tempents.RemoveCallback(id) end


convar = {}

--Returns ConVar value as string
---@param name string ConVar name
---@return string
function convar.GetString(name) end

--Returns ConVar value as number
---@param name string ConVar name
---@return number
function convar.GetNumber(name) end

--Returns ConVar value as boolean
---@param name string ConVar name
---@return boolean
function convar.GetBoolean(name) end

--Sets ConVar value
---@param name string ConVar name
---@param value string|number|boolean ConVar value
---@return nil
function convar.SetValue(name, value) end

--Checks if convar exists and is allowed to set
---@param name string ConVar name
---@return boolean `true` if convar can be set, `false` otherwise
function convar.IsValid(name) end

--Get client ConVar value
---@param player Entity Player to get the value from
---@param name string Name of the ConVar
---@return string value Value of the client ConVar
function convar.GetClientValue(player, name) end

--Set client ConVar value for bots
---@param bot Entity Bot to get the value from
---@param name string Name of the ConVar
---@param value string Value of the ConVar
---@return nil
function convar.SetFakeClientValue(bot, name, value) end


precache = {}

--Precaches model with specified path if not precached yet
---@param name string path to the model file
---@param preload? boolean = `false`. If the model should be preloaded
---@return number index model index
function precache.PrecacheModel(name, preload) end

--Precaches soundscript with specified name if not precached yet
---@param name string name of the soundscript
---@return number handle sound handle index
function precache.PrecacheScriptSound(name) end

--Precaches sound file with specified path if not precached yet
---@param name string path to the sound file
---@param preload? boolean = `false`. If the sound should be preloaded
---@return boolean success `true` on success, `false` on failure
function precache.PrecacheSound(name, preload) end

--Precaches sentence file with specified path if not precached yet
---@param name string path to the sentence file
---@param preload? boolean = `false`. If the sentence file should be preloaded
---@return number index sentence file index
function precache.PrecacheSentenceFile(name, preload) end

--Precaches decal file with specified path if not precached yet
---@param name string path to the decal file
---@param preload? boolean = `false`. If the decal should be preloaded
---@return number index decal file index
function precache.PrecacheDecal(name, preload) end

--Precaches generic file with specified path if not precached yet
---@param name string path to the generic file
---@param preload? boolean = `false`. If the generic file should be preloaded
---@return number index generic file index
function precache.PrecacheGeneric(name, preload) end

--Precaches particle with specified name if not precached yet
---@param name string name of the particle
---@return nil
function precache.PrecacheParticle(name) end


--Returns time in seconds since map load
---@return number
function CurTime() end

--Returns tick count since map load
---@return number
function TickCount() end

--Returns current map name
---@return string
function GetMapName() end

--Adds game event callback. See https://wiki.alliedmods.net/Team_Fortress_2_Events
---@param name string Name of the event
---@param callback function Callback function with parameters: eventTable (contains event key values, absent keys return 0). Return ACTION_MODIFY to send event to clients with modified values, ACTION_STOP to stop the event from being send to clients.
---@return number id id for later removal with `RemoveEventCallback(id)` function
function AddEventCallback(name, callback) end

--Removes game event callback
---@param id number id of the event callback
---@return nil
function RemoveEventCallback(id) end

--Fires a game event to clients. See https://wiki.alliedmods.net/Team_Fortress_2_Events
---@param name string Name of the event
---@param props table Table that contains property names and their values
---@return boolean success `true` on success, `false` otherwise
function FireEvent(name, props) end

--Adds global callback. Can be used to add multiple OnGameTick, OnWaveStart, etc. callbacks
---@param name string Name of the callback
---@param callback function Callback function
---@return number id id for later removal with `RemoveGlobalCallback(id)` function
function AddGlobalCallback(name, callback) end

--Removes global callback
---@param id number id of the global callback
---@return nil
function RemoveGlobalCallback(id) end

-- Table for transfering data between missions and maps. Cannot store other tables
DataTransfer = {}