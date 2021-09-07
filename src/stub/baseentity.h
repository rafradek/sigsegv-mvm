#ifndef _INCLUDE_SIGSEGV_STUB_BASEENTITY_H_
#define _INCLUDE_SIGSEGV_STUB_BASEENTITY_H_


#include "link/link.h"
#include "prop.h"


// TODO
class CGlobalEntityList : public CBaseEntityList {};


using BASEPTR       = void (CBaseEntity::*)();
using ENTITYFUNCPTR = void (CBaseEntity::*)(CBaseEntity *pOther);
using USEPTR        = void (CBaseEntity::*)(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);


struct inputdata_t
{
	CBaseEntity *pActivator;
	CBaseEntity *pCaller;
	variant_t value;
	int nOutputID;
};


enum DebugOverlayBits_t : uint32_t
{
	OVERLAY_TEXT_BIT                 = 0x00000001, // show text debug overlay for this entity
	OVERLAY_NAME_BIT                 = 0x00000002, // show name debug overlay for this entity
	OVERLAY_BBOX_BIT                 = 0x00000004, // show bounding box overlay for this entity
	OVERLAY_PIVOT_BIT                = 0x00000008, // show pivot for this entity
	OVERLAY_MESSAGE_BIT              = 0x00000010, // show messages for this entity
	OVERLAY_ABSBOX_BIT               = 0x00000020, // show abs bounding box overlay
	OVERLAY_RBOX_BIT                 = 0x00000040, // show the rbox overlay
	OVERLAY_SHOW_BLOCKSLOS           = 0x00000080, // show entities that block NPC LOS
	OVERLAY_ATTACHMENTS_BIT          = 0x00000100, // show attachment points
	OVERLAY_AUTOAIM_BIT              = 0x00000200, // Display autoaim radius
	OVERLAY_NPC_SELECTED_BIT         = 0x00001000, // the npc is current selected
	OVERLAY_NPC_NEAREST_BIT          = 0x00002000, // show the nearest node of this npc
	OVERLAY_NPC_ROUTE_BIT            = 0x00004000, // draw the route for this npc
	OVERLAY_NPC_TRIANGULATE_BIT      = 0x00008000, // draw the triangulation for this npc
	OVERLAY_NPC_ZAP_BIT              = 0x00010000, // destroy the NPC
	OVERLAY_NPC_ENEMIES_BIT          = 0x00020000, // show npc's enemies
	OVERLAY_NPC_CONDITIONS_BIT       = 0x00040000, // show NPC's current conditions
	OVERLAY_NPC_SQUAD_BIT            = 0x00080000, // show npc squads
	OVERLAY_NPC_TASK_BIT             = 0x00100000, // show npc task details
	OVERLAY_NPC_FOCUS_BIT            = 0x00200000, // show line to npc's enemy and target
	OVERLAY_NPC_VIEWCONE_BIT         = 0x00400000, // show npc's viewcone
	OVERLAY_NPC_KILL_BIT             = 0x00800000, // kill the NPC, running all appropriate AI.
	OVERLAY_WC_CHANGE_ENTITY         = 0x01000000, // object changed during WC edit
	OVERLAY_BUDDHA_MODE              = 0x02000000, // take damage but don't die
	OVERLAY_NPC_STEERING_REGULATIONS = 0x04000000, // Show the steering regulations associated with the NPC
	OVERLAY_TASK_TEXT_BIT            = 0x08000000, // show task and schedule names when they start
	OVERLAY_PROP_DEBUG               = 0x10000000, 
	OVERLAY_NPC_RELATION_BIT         = 0x20000000, // show relationships between target and all children
	OVERLAY_VIEWOFFSET               = 0x40000000, // show view offset
};


class CBaseCombatCharacter;
class CBaseCombatWeapon;
class INextBot;

class CBaseEntityOutput
{
public:
	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 ) { ft_FireOutput(this, Value, pActivator, pCaller, fDelay); }

private:
	static MemberFuncThunk<CBaseEntityOutput *, void, variant_t, CBaseEntity *, CBaseEntity *, float> ft_FireOutput;
};

class CServerNetworkProperty : public IServerNetworkable
{
public:
	void MarkPVSInformationDirty()
	{
		if (this->GetEdict() != nullptr) {
			this->GetEdict()->m_fStateFlags |= FL_EDICT_DIRTY_PVS_INFORMATION;
		}
	}
	
	// ...
};


class CBaseEntity : public IServerEntity
{
public:
	/* inline */
	edict_t *edict() const { return this->NetworkProp()->GetEdict(); }
	int entindex() const;
	const Vector& GetAbsOrigin() const;
	const QAngle& GetAbsAngles() const;
	const Vector& GetAbsVelocity() const;
	bool IsEFlagSet(int nEFlagMask) const;
	void AddEFlags(int nEFlagMask);
	const matrix3x4_t& EntityToWorldTransform() const;
	bool NameMatches(const char *pszNameOrWildcard);
	void SetModel(const char *szModelName);
	bool ClassMatches(const char *pszClassOrWildcard);
	void RemoveEffects(int nEffects);
	void ClearEffects();
	bool HasSpawnFlags(int flags) const;
	
	/* getter/setter */
	IServerNetworkable *GetNetworkable() const    { return &this->m_Network; }
	CServerNetworkProperty *NetworkProp() const   { return &this->m_Network; }
	inline const char *GetClassname() const              { return STRING((string_t)this->m_iClassname); }
	string_t GetEntityName() const                { return this->m_iName; }
	void SetName(string_t newName)                { this->m_iName = newName; }
	ICollideable *GetCollideable() const          { return &this->m_Collision; }
	CCollisionProperty *CollisionProp() const     { return &this->m_Collision; }
	int GetTeamNumber() const                     { return this->m_iTeamNum; }
	void SetTeamNumber(int number)                { this->m_iTeamNum = number; }
	void SetMaxHealth(int amt)                    { this->m_iMaxHealth = amt; }
	int GetHealth() const                         { return this->m_iHealth; }
	void SetHealth(int amt)                       { this->m_iHealth = amt; }
	CBaseEntity *GetGroundEntity() const          { return this->m_hGroundEntity; }
	CBaseEntity *GetOwnerEntity() const           { return this->m_hOwnerEntity; }
	IPhysicsObject *VPhysicsGetObject() const     { return this->m_pPhysicsObject; }
	int GetFlags() const                          { return this->m_fFlags; }
	int GetCollisionGroup() const                 { return this->m_CollisionGroup; }
	void SetCollisionGroup(int group)             { this->m_CollisionGroup = group; }
	SolidType_t GetSolid() const                  { return this->CollisionProp()->GetSolid(); }
	//void SetSolid(SolidType_t solid)              { return this->CollisionProp()->SetSolid(solid); }
	model_t *GetModel() const                     { return const_cast<model_t *>(modelinfo->GetModel(this->GetModelIndex())); }
	bool IsTransparent() const                    { return (this->m_nRenderMode != kRenderNormal); }
	int GetRenderMode() const                     { return this->m_nRenderMode; }
	void SetRenderMode(int mode)                  { this->m_nRenderMode = mode; }
	MoveType_t GetMoveType() const                { return (MoveType_t)(unsigned char)this->m_MoveType; }
	MoveCollide_t GetMoveCollide() const          { return (MoveCollide_t)(unsigned char)this->m_MoveCollide; }
	void SetMoveCollide(MoveCollide_t val)        { this->m_MoveCollide = val; }
	CBaseEntity *GetMoveParent()                  { return this->m_hMoveParent; }
	CBaseEntity *FirstMoveChild()                 { return this->m_hMoveChild; }
	CBaseEntity *NextMovePeer()                   { return this->m_hMovePeer; }
	const color32 GetRenderColor() const          { return this->m_clrRender; }
	void SetRenderColorR(byte r)                  { this->m_clrRender->r = r; }
	void SetRenderColorG(byte g)                  { this->m_clrRender->g = g; }
	void SetRenderColorB(byte b)                  { this->m_clrRender->b = b; }
	void SetRenderColorA(byte a)                  { this->m_clrRender->a = a; }
	bool IsMarkedForDeletion() const              { return ((this->m_iEFlags & EFL_KILLME) != 0); }
	bool BlocksLOS() const              		  { return ((this->m_iEFlags & EFL_DONTBLOCKLOS) == 0); }
	void SetBlocksLOS(bool val)                   { if (val) this->m_iEFlags = this->m_iEFlags & ~EFL_DONTBLOCKLOS; else this->m_iEFlags = this->m_iEFlags | EFL_DONTBLOCKLOS;}
	float GetGravity() const                      { return this->m_flGravity; }
	void SetGravity(float gravity)                { this->m_flGravity = gravity; }
	const Vector& GetLocalVelocity() const        { return this->m_vecVelocity; }
	const Vector& GetLocalOrigin() const          { return this->m_vecOrigin; }
	const QAngle& GetLocalAngles() const          { return this->m_angRotation; }
	const QAngle& GetLocalAngularVelocity() const { return this->m_vecAngVelocity; }
	void SetLocalVelocity(Vector &vec)            { this->m_vecVelocity = vec; }
	void SetLocalOrigin(Vector &vec)              { this->m_vecOrigin = vec; }
	void SetLocalAngles(QAngle &ang)              { this->m_angRotation = ang; }
	void SetLocalAngularVelocity( QAngle &ang)    { this->m_vecAngVelocity = ang; }
	int GetEffects() const                        { return this->m_fEffects; }
	bool IsEffectActive(int nEffects) const       { return ((this->m_fEffects & nEffects) != 0); }
	/* thunk */
	void Remove()                                                                                                           {        ft_Remove                        (this); }
	void CalcAbsolutePosition()                                                                                             {        ft_CalcAbsolutePosition          (this); }
	void CalcAbsoluteVelocity()                                                                                             {        ft_CalcAbsoluteVelocity          (this); }
	bool NameMatchesComplex(const char *pszNameOrWildcard)                                                                  { return ft_NameMatchesComplex            (this, pszNameOrWildcard); }
	bool ClassMatchesComplex(const char *pszClassOrWildcard)                                                                { return ft_ClassMatchesComplex           (this, pszClassOrWildcard); }
	void SetAbsOrigin(const Vector& absOrigin)                                                                              {        ft_SetAbsOrigin                  (this, absOrigin); }
	void SetAbsAngles(const QAngle& absAngles)                                                                              {        ft_SetAbsAngles                  (this, absAngles); }
	void SetAbsVelocity(const Vector& absVelocity)                                                                          {        ft_SetAbsVelocity                (this, absVelocity); }
	void EmitSound(const char *soundname, float soundtime = 0.0f, float *duration = nullptr)                                {        ft_EmitSound_member1             (this, soundname, soundtime, duration); }
	void EmitSound(const char *soundname, HSOUNDSCRIPTHANDLE& handle, float soundtime = 0.0f, float *duration = nullptr)    {        ft_EmitSound_member2             (this, soundname, handle, soundtime, duration); }
	void StopSound(const char *soundname)                                                                                   {        ft_StopSound                     (this, soundname); }
	float GetNextThink(const char *szContext)                                                                               { return ft_GetNextThink                  (this, szContext); }
	bool IsBSPModel() const                                                                                                 { return ft_IsBSPModel                    (this); }
	void EntityText(int text_offset, const char *text, float duration, int r, int g, int b, int a)                          {        ft_EntityText                    (this, text_offset, text, duration, r, g, b, a); }
	int TakeDamage(const CTakeDamageInfo& info)                                                                             { return ft_TakeDamage                    (this, info); }
	void SetMoveType(MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT)                                       {        ft_SetMoveType                   (this, val, moveCollide); }
	model_t *GetModel()                                                                                                     { return ft_GetModel                      (this); }
	void SetNextThink(float nextThinkTime, const char *szContext = nullptr)                                                 {        ft_SetNextThink_name             (this, nextThinkTime, szContext); }
	void SetNextThink(int nContextIndex, float thinkTime)                                                                   {        ft_SetNextThink_index            (this, nContextIndex, thinkTime); }
	int DispatchUpdateTransmitState()                                                                                       { return ft_DispatchUpdateTransmitState   (this); }
	void SetEffects(int nEffects)                                                                                           {        ft_SetEffects                    (this, nEffects); }
	void AddEffects(int nEffects)                                                                                           {        ft_AddEffects                    (this, nEffects); }
	bool ReadKeyField(const char *name, variant_t *var)                                                                     { return ft_ReadKeyField                  (this, name, var); }
	IPhysicsObject *VPhysicsInitStatic()                                                                                    { return ft_VPhysicsInitStatic            (this); }
	
	Vector EyePosition()                                                                                                    { return vt_EyePosition                   (this); }
	const QAngle& EyeAngles()                                                                                               { return vt_EyeAngles                     (this); }
	void SetOwnerEntity(CBaseEntity *pOwner)                                                                                {        vt_SetOwnerEntity                (this, pOwner); }
	void Spawn()                                                                                                            {        vt_Spawn                         (this); }
	void Activate()                                                                                                         {        vt_Activate                      (this); }
	void GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity = nullptr)                                             {        vt_GetVelocity                   (this, vVelocity, vAngVelocity); }
	const Vector& WorldSpaceCenter() const                                                                                  { return vt_WorldSpaceCenter              (this); }
	bool IsBaseCombatWeapon() const                                                                                         { return vt_IsBaseCombatWeapon            (this); }
	bool IsWearable() const                                                                                                 { return vt_IsWearable                    (this); }
	bool IsCombatItem() const                                                                                               { return vt_IsCombatItem                  (this); }
	void SetModelIndex(int index)                                                                                           {        vt_SetModelIndex                 (this, index); }
	int GetModelIndex() const                                                                                               { return vt_GetModelIndex                 (this); }
	string_t GetModelName() const                                                                                           { return vt_GetModelName                  (this); }
	CBaseCombatCharacter *MyCombatCharacterPointer()                                                                        { return vt_MyCombatCharacterPointer      (this); }
	CBaseCombatWeapon *MyCombatWeaponPointer()                                                                              { return vt_MyCombatWeaponPointer         (this); }
	bool ShouldCollide(int collisionGroup, int contentsMask) const                                                          { return vt_ShouldCollide                 (this, collisionGroup, contentsMask); }
	void DrawDebugGeometryOverlays()                                                                                        {        vt_DrawDebugGeometryOverlays     (this); }
	void ChangeTeam(int iTeamNum)                                                                                           {        vt_ChangeTeam                    (this, iTeamNum); }
	void SetModelIndexOverride(int index, int nValue)                                                                       {        vt_SetModelIndexOverride         (this, index, nValue); }
	datamap_t *GetDataDescMap()                                                                                             { return vt_GetDataDescMap                (this); }
	bool AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID) { return vt_AcceptInput                   (this, szInputName, pActivator, pCaller, Value, outputID); }
	float GetDamage()                                                                                                       { return vt_GetDamage                     (this); }
	void SetDamage(float flDamage)                                                                                          {        vt_SetDamage                     (this, flDamage); }
	bool FVisible(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = nullptr)                   { return vt_FVisible_ent                  (this, pEntity, traceMask, ppBlocker); }
	bool FVisible(const Vector& vecTarget, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = nullptr)                { return vt_FVisible_vec                  (this, vecTarget, traceMask, ppBlocker); }
	void Touch(CBaseEntity *pOther)                                                                                         {        vt_Touch                         (this, pOther); }
	INextBot *MyNextBotPointer()                                                                                            { return vt_MyNextBotPointer              (this); }
	void Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity)                            {        vt_Teleport                      (this, newPosition, newAngles, newVelocity); }
	int GetMaxHealth() const                                                                                                { return vt_GetMaxHealth                  (this); }
	bool IsAlive()                                                                                                          { return vt_IsAlive                       (this); }
	float GetDefaultItemChargeMeterValue() const                                                                            { return vt_GetDefaultItemChargeMeterValue(this); }
	bool IsDeflectable()																									{ return vt_IsDeflectable                 (this); }
	void SetParent(CBaseEntity *entity, int attachment)                                                                     {        vt_SetParent                     (this, entity, attachment); }
	bool IsPlayer()	const																									{ return vt_IsPlayer                      (this); }
	bool IsBaseObject() const																								{ return vt_IsBaseObject                  (this); }
	bool KeyValue(const char *key, const char *value)                                                                       { return vt_KeyValue                      (this, key, value); }
	bool GetKeyValue(const char *key, char *value, int maxlen)                                                              { return vt_GetKeyValue                   (this, key, value, maxlen); }
	void FireBullets(const FireBulletsInfo_t &info)                                                                         {        vt_FireBullets                   (this, info); }
	
	/* static */
	static CBaseEntity *Create(const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner = nullptr)                                                                       { return ft_Create             (szName, vecOrigin, vecAngles, pOwner); }
	static CBaseEntity *CreateNoSpawn(const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner = nullptr)                                                                { return ft_CreateNoSpawn      (szName, vecOrigin, vecAngles, pOwner); }
	static int PrecacheModel(const char *name, bool bPreload = true)                                                                                                                                      { return ft_PrecacheModel      (name, bPreload); }
	static bool PrecacheSound(const char *name)                                                                                                                                                           { return ft_PrecacheSound      (name); }
	static HSOUNDSCRIPTHANDLE PrecacheScriptSound(const char *soundname)                                                                                                                                  { return ft_PrecacheScriptSound(soundname); }
	static void EmitSound(IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin = nullptr, float soundtime = 0.0f, float *duration = nullptr)                             {        ft_EmitSound_static1  (filter, iEntIndex, soundname, pOrigin, soundtime, duration); }
	static void EmitSound(IRecipientFilter& filter, int iEntIndex, const char *soundname, HSOUNDSCRIPTHANDLE& handle, const Vector *pOrigin = nullptr, float soundtime = 0.0f, float *duration = nullptr) {        ft_EmitSound_static2  (filter, iEntIndex, soundname, handle, pOrigin, soundtime, duration); }
	static void EmitSound(IRecipientFilter& filter, int iEntIndex, const EmitSound_t& params)                                                                                                             {        ft_EmitSound_static3  (filter, iEntIndex, params); }
	static void EmitSound(IRecipientFilter& filter, int iEntIndex, const EmitSound_t& params, HSOUNDSCRIPTHANDLE& handle)                                                                                 {        ft_EmitSound_static4  (filter, iEntIndex, params, handle); }
	
	/* hack */
	bool IsCombatCharacter() { return (this->MyCombatCharacterPointer() != nullptr); }
	// bool IsPlayer() const;
	// bool IsBaseObject() const;
	
	/* also a hack */
	template<typename DERIVEDPTR> BASEPTR ThinkSet(DERIVEDPTR func, float flNextThinkTime = 0.0f, const char *szContext = nullptr);
	
	/* network vars */
	void NetworkStateChanged();
	void NetworkStateChanged(void *pVar);
	
	DECL_DATAMAP(string_t, m_target);
	DECL_DATAMAP(string_t, m_iParent);
	DECL_DATAMAP(int,      m_debugOverlays);
	
	/* TODO: make me private again! */
	DECL_SENDPROP(int,    m_fFlags);
	DECL_DATAMAP (int,    m_nNextThinkTick);
	DECL_SENDPROP(char,   m_lifeState);
	DECL_SENDPROP(int[4], m_nModelIndexOverrides);
	DECL_SENDPROP(bool,   m_iTextureFrameIndex);
	DECL_DATAMAP(float,      m_flLocalTime);
	DECL_DATAMAP(float,      m_flAnimTime);
	DECL_DATAMAP(float,      m_flSimulationTime);
	DECL_DATAMAP(float,      m_flVPhysicsUpdateLocalTime);
	DECL_DATAMAP(int,       m_spawnflags);
	DECL_DATAMAP(CBaseEntityOutput,      m_OnUser1);
	DECL_DATAMAP(CBaseEntityOutput,      m_OnUser2);
	DECL_DATAMAP(CBaseEntityOutput,      m_OnUser3);
	DECL_DATAMAP(CBaseEntityOutput,      m_OnUser4);
	DECL_DATAMAP(char,       m_takedamage);
	
	
	
	
private:
	DECL_DATAMAP(CServerNetworkProperty, m_Network);
	DECL_DATAMAP(string_t,               m_iClassname);
	DECL_DATAMAP(string_t,               m_iName);
	DECL_DATAMAP(int,                    m_iEFlags);
	DECL_DATAMAP(Vector,                 m_vecAbsOrigin);
	DECL_DATAMAP(QAngle,                 m_angAbsRotation);
	DECL_DATAMAP(Vector,                 m_vecAbsVelocity);
	DECL_DATAMAP(IPhysicsObject *,       m_pPhysicsObject);
	DECL_DATAMAP(matrix3x4_t,            m_rgflCoordinateFrame);
	DECL_DATAMAP(CHandle<CBaseEntity>,   m_hMoveChild);
	DECL_DATAMAP(CHandle<CBaseEntity>,   m_hMovePeer);
	DECL_DATAMAP(CHandle<CBaseEntity>,   m_hMoveParent);
	DECL_DATAMAP(float,                  m_flGravity);
	DECL_DATAMAP(QAngle,                 m_vecAngVelocity);
	
	DECL_SENDPROP_RW(CCollisionProperty,   m_Collision);
	DECL_SENDPROP   (int,                  m_iTeamNum);
	DECL_SENDPROP   (int,                  m_iMaxHealth);
	DECL_SENDPROP   (int,                  m_iHealth);
	DECL_SENDPROP   (CHandle<CBaseEntity>, m_hGroundEntity);
	DECL_SENDPROP   (CHandle<CBaseEntity>, m_hOwnerEntity);
	DECL_SENDPROP   (int,                  m_CollisionGroup);
	DECL_SENDPROP   (unsigned char,        m_nRenderMode);
	DECL_SENDPROP   (unsigned char,        m_MoveType);
	DECL_SENDPROP   (unsigned char,        m_MoveCollide);
	DECL_SENDPROP_RW(color32,              m_clrRender);
	DECL_SENDPROP   (Vector,               m_vecVelocity);
	DECL_SENDPROP   (Vector,               m_vecOrigin);
	DECL_SENDPROP   (QAngle,               m_angRotation);
	DECL_SENDPROP_RW(int,                  m_fEffects);
	
	static MemberFuncThunk<      CBaseEntity *, void>                                                    ft_Remove;
	static MemberFuncThunk<      CBaseEntity *, void>                                                    ft_CalcAbsolutePosition;
	static MemberFuncThunk<      CBaseEntity *, void>                                                    ft_CalcAbsoluteVelocity;
	static MemberFuncThunk<      CBaseEntity *, bool, const char *>                                      ft_NameMatchesComplex;
	static MemberFuncThunk<      CBaseEntity *, bool, const char *>                                      ft_ClassMatchesComplex;
	static MemberFuncThunk<      CBaseEntity *, void, const Vector&>                                     ft_SetAbsOrigin;
	static MemberFuncThunk<      CBaseEntity *, void, const QAngle&>                                     ft_SetAbsAngles;
	static MemberFuncThunk<      CBaseEntity *, void, const Vector&>                                     ft_SetAbsVelocity;
	static MemberFuncThunk<      CBaseEntity *, void, const char *, float, float *>                      ft_EmitSound_member1;
	static MemberFuncThunk<      CBaseEntity *, void, const char *, HSOUNDSCRIPTHANDLE&, float, float *> ft_EmitSound_member2;
	static MemberFuncThunk<      CBaseEntity *, void, const char *>                                      ft_StopSound;
	static MemberFuncThunk<      CBaseEntity *, float, const char *>                                     ft_GetNextThink;
	static MemberFuncThunk<      CBaseEntity *, void, const Vector&, Vector *>                           ft_EntityToWorldSpace;
	static MemberFuncThunk<const CBaseEntity *, bool>                                                    ft_IsBSPModel;
	static MemberFuncThunk<      CBaseEntity *, void, int, const char *, float, int, int, int, int>      ft_EntityText;
	static MemberFuncThunk<      CBaseEntity *, int, const CTakeDamageInfo&>                             ft_TakeDamage;
	static MemberFuncThunk<      CBaseEntity *, void, MoveType_t, MoveCollide_t>                         ft_SetMoveType;
	static MemberFuncThunk<      CBaseEntity *, model_t *>                                               ft_GetModel;
	static MemberFuncThunk<      CBaseEntity *, void, float, const char *>                               ft_SetNextThink_name;
	static MemberFuncThunk<      CBaseEntity *, void, int, float>                                        ft_SetNextThink_index;
	static MemberFuncThunk<      CBaseEntity *, BASEPTR, BASEPTR, float, const char *>                   ft_ThinkSet;
	static MemberFuncThunk<      CBaseEntity *, int>                                                     ft_DispatchUpdateTransmitState;
	static MemberFuncThunk<      CBaseEntity *, void, int>                                               ft_SetEffects;
	static MemberFuncThunk<      CBaseEntity *, void, int>                                               ft_AddEffects;
	static MemberFuncThunk<      CBaseEntity *, bool, const char *, variant_t *>                         ft_ReadKeyField;
	static MemberFuncThunk<      CBaseEntity *, IPhysicsObject *>                                        ft_VPhysicsInitStatic;
	
	static MemberVFuncThunk<      CBaseEntity *, Vector>                                                           vt_EyePosition;
	static MemberVFuncThunk<      CBaseEntity *, const QAngle&>                                                    vt_EyeAngles;
	static MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *>                                              vt_SetOwnerEntity;
	static MemberVFuncThunk<      CBaseEntity *, void>                                                             vt_Spawn;
	static MemberVFuncThunk<      CBaseEntity *, void>                                                             vt_Activate;
	static MemberVFuncThunk<      CBaseEntity *, void, Vector *, AngularImpulse *>                                 vt_GetVelocity;
	static MemberVFuncThunk<const CBaseEntity *, const Vector&>                                                    vt_WorldSpaceCenter;
	static MemberVFuncThunk<const CBaseEntity *, bool>                                                             vt_IsBaseCombatWeapon;
	static MemberVFuncThunk<const CBaseEntity *, bool>                                                             vt_IsWearable;
	static MemberVFuncThunk<const CBaseEntity *, bool>                                                             vt_IsCombatItem;
	static MemberVFuncThunk<      CBaseEntity *, void, int>                                                        vt_SetModelIndex;
	static MemberVFuncThunk<const CBaseEntity *, int>                                                              vt_GetModelIndex;
	static MemberVFuncThunk<const CBaseEntity *, string_t>                                                         vt_GetModelName;
	static MemberVFuncThunk<      CBaseEntity *, CBaseCombatCharacter *>                                           vt_MyCombatCharacterPointer;
	static MemberVFuncThunk<      CBaseEntity *, CBaseCombatWeapon *>                                              vt_MyCombatWeaponPointer;
	static MemberVFuncThunk<const CBaseEntity *, bool, int, int>                                                   vt_ShouldCollide;
	static MemberVFuncThunk<      CBaseEntity *, void>                                                             vt_DrawDebugGeometryOverlays;
	static MemberVFuncThunk<      CBaseEntity *, void, int>                                                        vt_ChangeTeam;
	static MemberVFuncThunk<      CBaseEntity *, void, int, int>                                                   vt_SetModelIndexOverride;
	static MemberVFuncThunk<      CBaseEntity *, datamap_t *>                                                      vt_GetDataDescMap;
	static MemberVFuncThunk<      CBaseEntity *, bool, const char *, CBaseEntity *, CBaseEntity *, variant_t, int> vt_AcceptInput;
	static MemberVFuncThunk<      CBaseEntity *, void, const char *>                                               vt_SetModel;
	static MemberVFuncThunk<      CBaseEntity *, float>                                                            vt_GetDamage;
	static MemberVFuncThunk<      CBaseEntity *, void, float>                                                      vt_SetDamage;
	static MemberVFuncThunk<      CBaseEntity *, bool, CBaseEntity *, int, CBaseEntity **>                         vt_FVisible_ent;
	static MemberVFuncThunk<      CBaseEntity *, bool, const Vector&, int, CBaseEntity **>                         vt_FVisible_vec;
	static MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *>                                              vt_Touch;
	static MemberVFuncThunk<      CBaseEntity *, INextBot *>                                                       vt_MyNextBotPointer;
	static MemberVFuncThunk<      CBaseEntity *, void, const Vector *, const QAngle *, const Vector *>             vt_Teleport;
	static MemberVFuncThunk<const CBaseEntity *, int>                                                              vt_GetMaxHealth;
	static MemberVFuncThunk<      CBaseEntity *, bool>                                                             vt_IsAlive;
	static MemberVFuncThunk<const CBaseEntity *, float>                                                            vt_GetDefaultItemChargeMeterValue;
	static MemberVFuncThunk<      CBaseEntity *, bool>                                                             vt_IsDeflectable;
	static MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *, int>                                         vt_SetParent;
	static MemberVFuncThunk<const CBaseEntity *, bool>                                                             vt_IsPlayer;
	static MemberVFuncThunk<const CBaseEntity *, bool>                                                             vt_IsBaseObject;
	static MemberVFuncThunk<      CBaseEntity *, bool, const char*, const char*>                                   vt_KeyValue;
	static MemberVFuncThunk<      CBaseEntity *, bool, const char*, char*, int>                                    vt_GetKeyValue;
	static MemberVFuncThunk<      CBaseEntity *, void, const FireBulletsInfo_t &>                                  vt_FireBullets;
	

	static StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        ft_Create;
	static StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        ft_CreateNoSpawn;
	static StaticFuncThunk<int, const char *, bool>                                                                         ft_PrecacheModel;
	static StaticFuncThunk<bool, const char *>                                                                              ft_PrecacheSound;
	static StaticFuncThunk<HSOUNDSCRIPTHANDLE, const char *>                                                                ft_PrecacheScriptSound;
	static StaticFuncThunk<void, IRecipientFilter&, int, const char *, const Vector *, float, float *>                      ft_EmitSound_static1;
	static StaticFuncThunk<void, IRecipientFilter&, int, const char *, HSOUNDSCRIPTHANDLE&, const Vector *, float, float *> ft_EmitSound_static2;
	static StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&>                                                ft_EmitSound_static3;
	static StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&, HSOUNDSCRIPTHANDLE&>                           ft_EmitSound_static4;
};


inline CBaseEntity *GetContainingEntity(edict_t *pent)
{
	if (pent != nullptr && pent->GetUnknown() != nullptr) {
		return pent->GetUnknown()->GetBaseEntity();
	}
	
	return nullptr;
}

inline int ENTINDEX(const edict_t *pEdict)
{
	return gamehelpers->IndexOfEdict(const_cast<edict_t *>(pEdict));
}

inline int ENTINDEX(const CBaseEntity *pEnt)
{
	if (pEnt == nullptr) {
		return 0;
	}
	
	return pEnt->entindex();
}

inline edict_t *INDEXENT(int iEdictNum)
{
	return engine->PEntityOfEntIndex(iEdictNum);
}

inline bool FNullEnt(const edict_t *pent)
{
	return (pent == nullptr || ENTINDEX(pent) == 0);
}

inline CBaseEntity *UTIL_EntityByIndex(int entityIndex)
{
	CBaseEntity *entity = nullptr;
	
	if (entityIndex > 0) {
		edict_t *edict = INDEXENT(entityIndex);
		if (edict != nullptr && !edict->IsFree()) {
			entity = GetContainingEntity(edict);
		}
	}
	
	return entity;
}


inline int CBaseEntity::entindex() const
{
	return ENTINDEX(this->GetNetworkable()->GetEdict());
}

inline const Vector& CBaseEntity::GetAbsOrigin() const
{
	if (this->IsEFlagSet(EFL_DIRTY_ABSTRANSFORM)) {
		const_cast<CBaseEntity *>(this)->CalcAbsolutePosition();
	}
	return this->m_vecAbsOrigin;
}

inline const QAngle& CBaseEntity::GetAbsAngles() const
{
	if (this->IsEFlagSet(EFL_DIRTY_ABSTRANSFORM)) {
		const_cast<CBaseEntity *>(this)->CalcAbsolutePosition();
	}
	return this->m_angAbsRotation;
}

inline const Vector& CBaseEntity::GetAbsVelocity() const
{
	if (this->IsEFlagSet(EFL_DIRTY_ABSVELOCITY)) {
		const_cast<CBaseEntity *>(this)->CalcAbsoluteVelocity();
	}
	return this->m_vecAbsVelocity;
}

inline bool CBaseEntity::IsEFlagSet(int nEFlagMask) const
{
	return (this->m_iEFlags & nEFlagMask) != 0;
}

inline void CBaseEntity::AddEFlags(int nEFlagMask)
{
	this->m_iEFlags |= nEFlagMask;
	if (this->m_iEFlags & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX )) {
		this->DispatchUpdateTransmitState();
	}
}

inline bool CBaseEntity::HasSpawnFlags( int nFlags ) const
{ 
	return (this->m_spawnflags & nFlags) != 0; 
}

inline const matrix3x4_t& CBaseEntity::EntityToWorldTransform() const
{
	if (this->IsEFlagSet(EFL_DIRTY_ABSTRANSFORM)) {
		const_cast<CBaseEntity *>(this)->CalcAbsolutePosition();
	}
	return this->m_rgflCoordinateFrame;
}

inline bool CBaseEntity::NameMatches(const char *pszNameOrWildcard)
{
	return (IDENT_STRINGS(this->m_iName, pszNameOrWildcard) || this->NameMatchesComplex(pszNameOrWildcard));
}

inline void CBaseEntity::SetModel(const char *szModelName)
{
	/* ensure that we don't accidentally run into the fatal Error statement in
	 * UTIL_SetModel */
	CBaseEntity::PrecacheModel(szModelName);
	
	/* now do the vcall */
	vt_SetModel(this, szModelName);
}

inline bool CBaseEntity::ClassMatches(const char *pszClassOrWildcard)
{
	return (IDENT_STRINGS(this->m_iClassname, pszClassOrWildcard) || this->ClassMatchesComplex(pszClassOrWildcard));
}

inline void CBaseEntity::RemoveEffects(int nEffects)
{
	this->m_fEffects &= ~nEffects;
	
	if ((nEffects & EF_NODRAW) != 0) {
		this->NetworkProp()->MarkPVSInformationDirty();
		this->DispatchUpdateTransmitState();
	}
}

inline void CBaseEntity::ClearEffects()
{
	this->m_fEffects = 0;
	
	this->DispatchUpdateTransmitState();
}


/* like those stupid SetThink and SetContextThink macros, but way better! */
#define THINK_FUNC_DECL(name) \
	class ThinkFunc_##name : public CBaseEntity \
	{ \
	public: \
		void Update(); \
	}; \
	void ThinkFunc_##name::Update()

#define THINK_FUNC_SET(ent, name, time) ent->ThinkSet(&ThinkFunc_##name::Update, time, #name)

template<typename DERIVEDPTR>
BASEPTR CBaseEntity::ThinkSet(DERIVEDPTR func, float flNextThinkTime, const char *szContext)
{
	return ft_ThinkSet(this, static_cast<BASEPTR>(func), flNextThinkTime, szContext);
}

inline void CBaseEntity::NetworkStateChanged()
{
	gamehelpers->SetEdictStateChanged(this->GetNetworkable()->GetEdict(), 0);
}

inline void CBaseEntity::NetworkStateChanged(void *pVar)
{
	gamehelpers->SetEdictStateChanged(this->GetNetworkable()->GetEdict(), ((uintptr_t)pVar - (uintptr_t)this));
}


inline CBaseEntity *CreateEntityByName(const char *szClassname)
{
	return servertools->CreateEntityByName(szClassname);
}

inline void DispatchSpawn(CBaseEntity *pEntity)
{
	servertools->DispatchSpawn(pEntity);
}


inline void UTIL_Remove(CBaseEntity *pEntity)
{
	if (pEntity != nullptr) {
		pEntity->Remove();
	}
}


#endif
