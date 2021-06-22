#include <string>
#include <type_traits>
#include <functional>

#include <networkvar.h>
#include <game/server/cbase.h>
#include <mathlib/vector.h>
#include <mathlib/vmatrix.h>
#include <tier1/utlvector.h>
#include <game/shared/shareddefs.h>
#include <game/shared/ehandle.h>
#include <game/server/util.h>
#include <isaverestore.h>
#include <tier1/KeyValues.h>
#include <tier1/utlstring.h>
#include <tier1/convar.h>
#include <const.h>
#include <eiface.h>
#include <eifacev21.h>
#include <ihandleentity.h>
#include <iserverentity.h>
#include <iserverunknown.h>
#include <iservernetworkable.h>
#include <edict.h>
#include <game/server/ServerNetworkProperty.h>
#include <game/server/baseentity.h>
#include <igameevents.h>
#include <tier1/utlmemory.h>
#include <icvar.h>
#include <basehandle.h>
#include <game/shared/ehandle.h>
#include <game/shared/util_shared.h>
#include <vstdlib/random.h>
#include <tier1/fmtstr.h>
#include <string_t.h>
#include <tier1/strtools.h>
#include <gametrace.h>
#include <trace.h>
#include <tier0/vprof.h>
#include <game/server/nav.h>
#include <game/server/nav_area.h>
#include <game/server/nav_mesh.h>
#include <game/shared/teamplayroundbased_gamerules.h>
#include <game/server/eventqueue.h>
#include <game/shared/gamestringpool.h>
#include <game/server/team.h>


#define UNKNOWN int


#define TF_TEAM_RED 2
#define TF_TEAM_BLUE 3


enum
{
	TF_CLASS_UNDEFINED = 0,
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
};


class CTFBot;
class CTFPlayer;
class CPopulationManager;
class CSpawnLocation;
class IPopulationSpawner;
class Path;
class PathFollower;
class CBotNPCArcher;
class CBotNPCDecoy;
class CEyeballBoss;
class CGhost;
class CRobot;
class CSimpleBot;
class CHeadlessHatman;
class CMerasmus;
class CTFRobotDestruction_Robot;
class CZombie;
class CEconItem;
class CEconItemAttributeDefinition;
class IEconItemAttributeIterator;
class CTFBotHintEngineerNest;
class CTFTankBoss;
class CCaptureFlag;
class CWeaponMedigun;

template <class T> class Action;

class IHasAttributes {};
class IInventoryUpdateListener {};
class CBaseObject : public CBaseCombatCharacter {
public: void InitializeMapPlacedObject();
};
class CObjectSentrygun : public CBaseObject {};

int GetClassIndexFromString(const char *, int);
void *GetPlayerClassData( int iClass );

extern ConVar tf_populator_debug;
extern ConVar tf_mvm_miniboss_scale;
extern CPopulationManager *g_pPopulationManager;
extern char *g_aRawPlayerClassNamesShort[];



#include "NextBot/CKnownEntity.h"
#include "NextBot/IEventResponder.h"
#include "NextBot/INextBot.h"
#include "NextBot/NextBotPlayer.h"
#include "Population/Attribute.h"
#include "CTFBot/CTFBot.h"


inline CTFBot::AttributeType& operator|=(CTFBot::AttributeType& lhs, CTFBot::AttributeType rhs)
{
	lhs = (CTFBot::AttributeType)(static_cast<std::underlying_type_t<CTFBot::AttributeType>>(lhs) | static_cast<std::underlying_type_t<CTFBot::AttributeType>>(rhs));
	return lhs;
}

inline CTFBot::AttributeType operator&(CTFBot::AttributeType lhs, CTFBot::AttributeType rhs)
{
	return (CTFBot::AttributeType)(static_cast<std::underlying_type_t<CTFBot::AttributeType>>(lhs) & static_cast<std::underlying_type_t<CTFBot::AttributeType>>(rhs));
}


#include "NextBot/IContextualQuery.h"
#include "NextBot/IVision.h"
#include "NextBot/ILocomotion.h"
//#include "NextBot/IIntention.h"
#include "NextBot/IBody.h"
#include "NextBot/Behavior.h"
#include "NextBot/Action.h"
#include "Population/Event.h"
#include "Misc.h"
#include "Population/PopulationManager.h"
#include "Population/Populator.h"
#include "Population/Spawner.h"
