/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotKnownEntity.h
 */


class CKnownEntity
{
public:
	CKnownEntity(CBaseEntity *ent);
	virtual ~CKnownEntity();
	
	virtual void Destroy();
	virtual void UpdatePosition();
	virtual CBaseEntity *GetEntity() const;
	virtual const Vector *GetLastKnownPosition() const;
	virtual bool HasLastKnownPositionBeenSeen() const;
	virtual void MarkLastKnownPositionAsSeen();
	virtual CNavArea *GetLastKnownArea() const;
	virtual float GetTimeSinceLastKnown() const;
	virtual float GetTimeSinceBecameKnown() const;
	virtual void UpdateVisibilityStatus(bool visible);
	virtual bool IsVisibleInFOVNow() const;
	virtual bool IsVisibleRecently() const;
	virtual float GetTimeSinceBecameVisible() const;
	virtual float GetTimeWhenBecameVisible() const;
	virtual float GetTimeSinceLastSeen() const;
	virtual bool WasEverVisible() const;
	virtual bool IsObsolete() const;
	virtual bool operator==(const CKnownEntity& that) const;
	virtual bool Is(CBaseEntity *ent) const;
	
private:
	CHandle<CBaseEntity> m_hEntity;  // +0x04
	Vector m_Position;               // +0x08
	bool m_bLastKnownPosWasSeen;     // +0x14
	CNavArea *m_NavArea;             // +0x18
	float m_flTimeLastVisible;       // +0x1c
	float m_flTimeLastBecameVisible; // +0x20
	float m_flTimeLastKnown;         // +0x24
	float m_flTimeLastBecameKnown;   // +0x28
	bool m_bIsVisible;               // +0x2c
};
