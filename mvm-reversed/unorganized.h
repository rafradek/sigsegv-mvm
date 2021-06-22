class CHeadlessHatmanPathCost : public IPathCost
{
	// TODO
};

class CMerasmusPathCost : public IPathCost
{
	// TODO
};

class CRobotPathCost : public IPathCost
{
	// TODO
};

class CZombiePathCost : public IPathCost
{
	// TODO
};


inline CBasePlayer *ToBasePlayer(CBaseEntity *pEntity)
{
	if (pEntity == nullptr || !pEntity->IsPlayer()) {
		return nullptr;
	}
	
	return static_cast<CBasePlayer *>(pEntity);
}

inline CTFPlayer *ToTFPlayer(CBaseEntity *pEntity)
{
	if (pEntity == nullptr || !pEntity->IsPlayer()) {
		return nullptr;
	}
	
	return static_cast<CTFPlayer *>(pEntity);
}

inline CTFBot *ToTFBot(CBaseEntity *pEntity)
{
	CTFPlayer *player = ToTFPlayer(pEntity);
	
	if (player == nullptr || !player->IsBotOfType(1337)) {
		return nullptr;
	}
	
	return static_cast<CTFBot *>(player);
}


class CHeadlessHatmanBody : public IBody
{
	// TODO
};


class CMerasmusBody : public IBody
{
	// TODO
};


class CBotNPCBody : public IBody
{
	// TODO
};

class CEyeballBossBody : public CBotNPCBody
{
	// TODO
};


class CBotNPCArcher::CBotNPCArcherIntention : public IIntention
{
	// TODO
};


class CBotNPCDecoy::CBotNPCDecoyIntention : public IIntention
{
	// TODO
};


class CEyeballBoss::CEyeballBossIntention : public IIntention
{
	// TODO
};


class CGhost::CGhostIntention : public IIntention
{
	// TODO
};


class CHeadlessHatmanIntention : public IIntention
{
	// TODO
};


class CMerasmusIntention : public IIntention
{
	// TODO
};


class CRobot::CRobotIntention : public IIntention
{
	// TODO
};


class CZombieIntention : public IIntention
{
	// TODO
};


class CEyeballBossLocomotion : public ILocomotion
{
	// TODO
};


class CMerasmusFlyingLocomotion : public ILocomotion
{
	// TODO
};


class CBotNPCDecoyLocomotion : public NextBotGroundLocomotion
{
	// TODO
};

class CGhostLocomotion : public NextBotGroundLocomotion
{
	// TODO
};

class CHeadlessHatmanLocomotion : public NextBotGroundLocomotion
{
	// TODO
};

class CMerasmusLocomotion : public NextBotGroundLocomotion
{
	// TODO
};

class CRobotLocomotion : public NextBotGroundLocomotion
{
	// TODO
};

class CZombieLocomotion : public NextBotGroundLocomotion
{
	// TODO
};


class CDisableVision : public IVision
{
	// TODO
};


union attribute_data_union_t
{
	// TODO
};


class ISchemaAttributeType
{
public:
	virtual ~ISchemaAttributeType();
	
	UNKNOWN GetTypeUniqueIdentifier();
	UNKNOWN LoadEconAttributeValue(CEconItem *, CEconItemAttributeDefinition const*, attribute_data_union_t const&);
	UNKNOWN ConvertEconAttributeValueToByteStream(attribute_data_union_t const&, std::string *);
	UNKNOWN BConvertStringToEconAttributeValue(CEconItemAttributeDefinition const*, const char *, attribute_data_union_t *, bool);
	UNKNOWN ConvertEconAttributeValueToString(CEconItemAttributeDefinition const*, attribute_data_union_t const&, std::string *);
	UNKNOWN LoadByteStreamToEconAttributeValue(CEconItem *, CEconItemAttributeDefinition const*, std::string const&);
	UNKNOWN InitializeNewEconAttributeValue(attribute_data_union_t *);
	UNKNOWN UnloadEconAttributeValue(attribute_data_union_t *);
	UNKNOWN OnIterateAttributeValue(IEconItemAttributeIterator *, CEconItemAttributeDefinition const*, attribute_data_union_t const&);
	UNKNOWN BSupportsGameplayModificationAndNetworking();
};


struct static_attrib_t
{
	bool BInitFromKV_SingleLine(const char *context, KeyValues *kv, CUtlVector<CUtlString> *errors, bool integer);
	
	unsigned short m_iAttrIndex;    // +0x00
	attribute_data_union_t m_Value; // +0x04
};


// TODO: fix these to not conflict with the definitions in shareddefs.h
#define COLOR_WHITE    Color(255, 255, 255, 255)
#define COLOR_GRAY100  Color(100, 100, 100, 255)
#define COLOR_RED      Color(255,   0,   0, 255)
#define COLOR_ORANGE   Color(255, 100,   0, 255)
#define COLOR_LTYELLOW Color(255, 255, 150, 255)
#define COLOR_GREEN    Color(  0, 255,   0, 255)
#define COLOR_LTGREEN  Color(150, 255, 150, 255)
#define COLOR_MAGENTA  Color(255,   0, 255, 255)

#define COLOR32_WHITE { .r = 255, .g = 255, .b = 255, .a = 255, }

#define RGB_GRAY100  100, 100, 100
#define RGB_YELLOW   255, 255,   0
#define RGB_LTYELLOW 255, 255, 100

#define RGBA_WHITE   255, 255, 255, 255
#define RGBA_RED     255,   0,   0, 255
#define RGBA_GREEN     0, 255,   0, 255
#define RGBA_CYAN      0, 255, 255, 255
#define RGBA_MAGENTA 255,   0, 255, 255


class INextBotEntityFilter
{
public:
	virtual bool IsAllowed(CBaseEntity *ent) const = 0;
};


class INextBotFilter
{
public:
	virtual bool IsSelected(const CBaseEntity *ent) const = 0;
};


class INextBotReply
{
public:
	enum class FailureReason : int
	{
		REJECTED      = 0, // AimHeadTowards denied the aim
		PREEMPTED     = 1, // a higher priority aim preempted our aim
		UNIMPLEMENTED = 2, // subclass didn't override IBody::AimHeadTowards
	};
	
	virtual void OnSuccess(INextBot *nextbot);
	virtual void OnFail(INextBot *nextbot, FailureReason reason);
};


class PressFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressAltFireButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressFireButtonIfValidBuildPositionReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};

class PressJumpButtonReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override;
};


bool IgnoreActorsTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask);

class NextBotTraceFilterIgnoreActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterIgnoreActors(CBaseEntity *ent)
		: CTraceFilterSimple(ent, COLLISION_GROUP_NONE,
			IgnoreActorsTraceFilterFunction)
	{
	}
};


class NextBotTraceFilterOnlyActors : public CTraceFilterSimple
{
public:
	NextBotTraceFilterOnlyActors(/* ??? */)
		: CTraceFilterSimple(/* ??? */)
	{
		// TODO
	}
	
	virtual bool ShouldHitEntity(IHandleEntity *pEntity, int contentsMask) override
	{
		if (!CTraceFilterSimple::ShouldHitEntity(pEntity, contentsMask)) {
			return false;
		}
		
		// TODO
	}
	
	virtual TraceType_t GetTraceType() const override { return TRACE_ENTITIES_ONLY; }
	
private:
	// TODO
};


class NextBotTraversableTraceFilter : public CTraceFilterSimple
{
public:
	NextBotTraversableTraceFilter(/* ??? */)
		: CTraceFilterSimple(/* ??? */)
	{
		// TODO
	}
	
	virtual bool ShouldHitEntity(IHandleEntity *pEntity, int contentsMask) override
	{
		// TODO
	}
	
private:
	// 10 INextBot *
	// 14 ILocomotion::TraverseWhenType
};


bool VisionTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask);

class NextBotVisionTraceFilter : public CTraceFilterSimple
{
public:
	NextBotVisionTraceFilter(CBaseEntity *ent)
		: CTraceFilterSimple(ent, COLLISION_GROUP_NONE,
			VisionTraceFilterFunction)
	{
	}
};


class GroundLocomotionCollisionTraceFilter : public CTraceFilterSimple
{
	// TODO
};


bool IgnoreActorsTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *ent = EntityFromEntityHandle(pHandleEntity);
	return !ent->IsCombatCharacter();
}

bool VisionTraceFilterFunction(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *ent = EntityFromEntityHandle(pHandleEntity);
	if (ent->IsCombatCharacter()) {
		return false;
	} else {
		return ent->BlocksLOS();
	}
}


/* I invented this function, because this particular idiom comes up frequently;
 * I can't seem to actually find it anywhere in the 2013 SDK or the 2007 leak,
 * so I don't know if it actually is an inlined function or just a copypasta */
inline int GetEnemyTeam(CBaseEntity *ent)
{
	int enemy_team = ent->GetTeamNumber();
	
	switch (enemy_team) {
	case TF_TEAM_RED:
		enemy_team = TF_TEAM_BLUE;
		break;
	case TF_TEAM_BLUE:
		enemy_team = TF_TEAM_RED;
		break;
	}
	
	return enemy_team;
}
// check out CTFPlayer::GetOpposingTFTeam!


class CClosestTFPlayer
{
public:
	CClosestTFPlayer(const Vector& origin, int teamnum = TEAM_ANY) :
		m_vecOrigin(origin), m_iTeamNum(teamnum) {}
	
	bool operator()(CBasePlayer *player)
	{
		// TODO
	}
	
	Vector m_vecOrigin;             // +0x00
	float m_flMinDist = FLT_MAX;    // +0x0c
	CTFPlayer *m_pPlayer = nullptr; // +0x10
	int m_iTeamNum;                 // +0x14
};


// WTF WTF WTF WTF WTF
void CBaseObject::UpdateOnRemove()
{
	// ...
	
	this->DestroyObject();
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() && this->GetTeamNumber() == TF_TEAM_BLUE) {
		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			CBaseObject *obj = static_cast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
			if (obj == this) continue;
			
			if (obj->GetType() == this->GetType()) {
				if (!obj->IsDying() && obj->GetBuilder() == nullptr) {
					obj->DetonateObject();
				}
			}
		}
	}
	
	// ...
}
