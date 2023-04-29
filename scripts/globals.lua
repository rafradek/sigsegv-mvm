----------------
-- Teams
----------------
TEAM_UNASSIGNED = 0
TEAM_SPECTATOR = 1
TEAM_RED = 2
TEAM_BLUE = 3
TEAM_HALLOWEEN = 5

----------------
-- Classes (m_iClass player field)
----------------
TF_CLASS_UNDEFINED = 0
TF_CLASS_SCOUT = 1
TF_CLASS_SNIPER = 2
TF_CLASS_SOLDIER = 3
TF_CLASS_DEMOMAN = 4
TF_CLASS_MEDIC = 5
TF_CLASS_HEAVYWEAPONS = 6
TF_CLASS_PYRO = 7
TF_CLASS_SPY = 8
TF_CLASS_ENGINEER = 9
TF_CLASS_CIVILIAN = 10

----------------
-- Collision groups
----------------
COLLISION_GROUP_NONE = 0
COLLISION_GROUP_DEBRIS = 1  	-- Collides with nothing but world and static stuff
COLLISION_GROUP_DEBRIS_TRIGGER = 2 -- Same as debris, but hits triggers
COLLISION_GROUP_INTERACTIVE_DEBRIS = 3 -- Collides with everything except other interactive debris or debris
COLLISION_GROUP_INTERACTIVE = 4	-- Collides with everything except interactive debris or debris
COLLISION_GROUP_PLAYER = 5
COLLISION_GROUP_BREAKABLE_GLASS = 6
COLLISION_GROUP_VEHICLE = 7
COLLISION_GROUP_PLAYER_MOVEMENT = 8  -- For HL2, same as Collision_Group_Player, for
                                    -- TF2, this filters out other players and CBaseObjects
COLLISION_GROUP_NPC = 9			-- Generic NPC group
COLLISION_GROUP_IN_VEHICLE = 10	-- for any entity inside a vehicle
COLLISION_GROUP_WEAPON = 11		-- for any weapons that need collision detection
COLLISION_GROUP_VEHICLE_CLIP = 12	-- vehicle clip brush to restrict vehicle movement
COLLISION_GROUP_PROJECTILE = 13	-- Projectiles!
COLLISION_GROUP_DOOR_BLOCKER = 14	-- Blocks entities not permitted to get near moving doors
COLLISION_GROUP_PASSABLE_DOOR = 15 -- Doors that the player shouldn't collide with
COLLISION_GROUP_DISSOLVING = 16	-- Things that are dissolving are in this group
COLLISION_GROUP_PUSHAWAY = 17	-- Nonsolid on client and server, pushaway in player code

COLLISION_GROUP_NPC_ACTOR = 18		-- Used so NPCs in scripts ignore the player.
COLLISION_GROUP_NPC_SCRIPTED = 19	-- USed for NPCs in scripts that should not collide with each other
TF_COLLISIONGROUP_GRENADES = 20
TFCOLLISION_GROUP_OBJECT = 21
TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT = 22
TFCOLLISION_GROUP_COMBATOBJECT = 23
TFCOLLISION_GROUP_ROCKETS = 24		-- Solid to players, but not player movement. ensures touch calls are originating from rocket
TFCOLLISION_GROUP_RESPAWNROOMS = 25
TFCOLLISION_GROUP_TANK = 26
TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS = 27

----------------
-- Hit groups
----------------
HITGROUP_GENERIC =	0
HITGROUP_HEAD =		1
HITGROUP_CHEST =	2
HITGROUP_STOMACH =	3
HITGROUP_LEFTARM =	4
HITGROUP_RIGHTARM =	5
HITGROUP_LEFTLEG =	6
HITGROUP_RIGHTLEG =	7
HITGROUP_GEAR =		10

----------------
-- Contents
----------------
CONTENTS_EMPTY			= 0		-- No contents

CONTENTS_SOLID			= 0x1		-- an eye is never valid in a solid
CONTENTS_WINDOW			= 0x2		-- translucent, but not watery (glass)
CONTENTS_AUX			= 0x4
CONTENTS_GRATE			= 0x8		-- alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
CONTENTS_SLIME			= 0x10
CONTENTS_WATER			= 0x20
CONTENTS_BLOCKLOS		= 0x40	-- block AI line of sight
CONTENTS_OPAQUE			= 0x80	-- things that cannot be seen through (may be non-solid though)
LAST_VISIBLE_CONTENTS	= 0x80

ALL_VISIBLE_CONTENTS  = (LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS-1))

CONTENTS_TESTFOGVOLUME	= 0x100
CONTENTS_UNUSED			= 0x200	

-- unused 
-- NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
-- if not visible, then grab from the bottom.
CONTENTS_UNUSED6		= 0x400

CONTENTS_TEAM1			= 0x800	-- per team contents used to differentiate collisions 
CONTENTS_TEAM2			= 0x1000	-- between players and objects on different teams

-- ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
CONTENTS_IGNORE_NODRAW_OPAQUE	= 0x2000

-- hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
CONTENTS_MOVEABLE		= 0x4000

-- remaining contents are non-visible, and don't eat brushes
CONTENTS_AREAPORTAL		= 0x8000

CONTENTS_PLAYERCLIP		= 0x10000
CONTENTS_MONSTERCLIP	= 0x20000

-- currents can be added to any other contents, and may be mixed
CONTENTS_CURRENT_0		= 0x40000
CONTENTS_CURRENT_90		= 0x80000
CONTENTS_CURRENT_180	= 0x100000
CONTENTS_CURRENT_270	= 0x200000
CONTENTS_CURRENT_UP		= 0x400000
CONTENTS_CURRENT_DOWN	= 0x800000

CONTENTS_ORIGIN			= 0x1000000	-- removed before bsping an entity

CONTENTS_MONSTER		= 0x2000000	-- should never be on a brush, only in game
CONTENTS_DEBRIS			= 0x4000000
CONTENTS_DETAIL			= 0x8000000	-- brushes to be added after vis leafs
CONTENTS_TRANSLUCENT	= 0x10000000	-- auto set if any surface has trans
CONTENTS_LADDER			= 0x20000000
CONTENTS_HITBOX			= 0x40000000	-- use accurate hitboxes on trace

----------------
-- Masks
----------------
MASK_ALL					= (0xFFFFFFFF) -- everything that is normally solid

MASK_SOLID					= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) -- everything that blocks player movement

MASK_PLAYERSOLID			= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) -- blocks npc movement

MASK_NPCSOLID				= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) -- water physics in these contents

MASK_WATER					= (CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME) -- everything that blocks lighting

MASK_OPAQUE					= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE) -- everything that blocks lighting, but with monsters added.

MASK_OPAQUE_AND_NPCS		= (MASK_OPAQUE|CONTENTS_MONSTER) -- everything that blocks line of sight for AI

MASK_BLOCKLOS				= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS) -- everything that blocks line of sight for AI plus NPCs

MASK_BLOCKLOS_AND_NPCS		= (MASK_BLOCKLOS|CONTENTS_MONSTER) -- everything that blocks line of sight for players

MASK_VISIBLE					= (MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE) -- everything that blocks line of sight for players, but with monsters added.

MASK_VISIBLE_AND_NPCS		= (MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE) -- bullets see these as solid

MASK_SHOT					= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX) -- non-raycasted weapons see this as solid = (includes grates)

MASK_SHOT_HULL				= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE) -- hits solids = (not grates) and passes through everything else

MASK_SHOT_PORTAL			= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER) -- everything normally solid, except monsters = (world+brush only)

MASK_SOLID_BRUSHONLY		= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE) -- everything normally solid for player movement, except monsters = (world+brush only)

MASK_PLAYERSOLID_BRUSHONLY	= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE) -- everything normally solid for npc movement, except monsters = (world+brush only)

MASK_NPCSOLID_BRUSHONLY		= (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) -- just the world, used for route rebuilding

MASK_NPCWORLDSTATIC			= (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) -- These are things that can split areaportals

MASK_SPLITAREAPORTAL		= (CONTENTS_WATER|CONTENTS_SLIME)

MASK_DEADSOLID				= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE) -- everything that blocks corpse movement

----------------
-- Surfaces
----------------
SURF_LIGHT		= 0x0001		-- value will hold the light strength
SURF_SKY2D		= 0x0002		-- don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
SURF_SKY		= 0x0004		-- don't draw, but add to skybox
SURF_WARP		= 0x0008		-- turbulent water warp
SURF_TRANS		= 0x0010
SURF_NOPORTAL	= 0x0020	-- the surface can not have a portal placed on it
SURF_TRIGGER	= 0x0040	-- FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
SURF_NODRAW		= 0x0080	-- don't bother referencing the texture

SURF_HINT		= 0x0100	-- make a primary bsp splitter

SURF_SKIP		= 0x0200	-- completely ignore, allowing non-closed brushes
SURF_NOLIGHT	= 0x0400	-- Don't calculate light
SURF_BUMPLIGHT	= 0x0800	-- calculate three lightmaps for the surface for bumpmapping
SURF_NOSHADOWS	= 0x1000	-- Don't receive shadows
SURF_NODECALS	= 0x2000	-- Don't receive decals
SURF_NOCHOP		= 0x4000	-- Don't subdivide patches on this surface 
SURF_HITBOX		= 0x8000	-- surface is part of a hitbox

----------------
-- Displacement surface flags
----------------
DISPSURF_FLAG_SURFACE		= (1<<0)
DISPSURF_FLAG_WALKABLE		= (1<<1)
DISPSURF_FLAG_BUILDABLE		= (1<<2)
DISPSURF_FLAG_SURFPROP1		= (1<<3)
DISPSURF_FLAG_SURFPROP2		= (1<<4)

----------------
-- Damage types
----------------
DMG_GENERIC =		0			-- generic damage -- do not use if you want players to flinch and bleed!
DMG_CRUSH =			(1 << 0)	-- crushed by falling or moving object. 
										-- NOTE: It's assumed crush damage is occurring as a result of physics collision, so no extra physics force is generated by crush damage.
										-- DON'T use DMG_CRUSH when damaging entities unless it's the result of a physics collision. You probably want DMG_CLUB instead.
DMG_BULLET =		(1 << 1)	-- shot
DMG_SLASH =			(1 << 2)	-- cut, clawed, stabbed
DMG_BURN =			(1 << 3)	-- heat burned
DMG_VEHICLE =		(1 << 4)	-- hit by a vehicle
DMG_FALL =			(1 << 5)	-- fell too far
DMG_BLAST =			(1 << 6)	-- explosive blast damage
DMG_CLUB =			(1 << 7)	-- crowbar, punch, headbutt
DMG_SHOCK =			(1 << 8)	-- electric shock
DMG_SONIC =			(1 << 9)	-- sound pulse shockwave
DMG_RADIUS_MAX =	(1 << 10)	-- Explosive damage has no falloff (100% regardless of distance from explosion center)
DMG_PREVENT_PHYSICS_FORCE =		(1 << 11)	-- Prevent a physics force
DMG_NEVERGIB =		(1 << 12)	-- with this bit OR'd in, no damage type will be able to gib victims upon death
DMG_ALWAYSGIB =		(1 << 13)	-- with this bit OR'd in, any damage type can be made to gib victims upon death.
DMG_DROWN =			(1 << 14)	-- Drowning


DMG_PARALYZE =		(1 << 15)	-- slows affected creature down
DMG_NERVEGAS =		(1 << 16)	-- nerve toxins, very bad
DMG_NOCLOSEDISTANCEMOD =(1 << 17)	-- damage rampup decreased to 20%
DMG_HALF_FALLOFF =	(1 << 18)	-- Explosive damage has falloff reduced in half (75% damage minimum instead of 50%)
DMG_DROWNRECOVER =	(1 << 19)	-- drowning recovery
DMG_CRITICAL =		(1 << 20)	-- critical damage
DMG_USEDISTANCEMOD =(1 << 21)	-- deals reduced or increased damage based on distance

DMG_REMOVENORAGDOLL =(1<<22)		-- with this bit OR'd in, no ragdoll will be created, and the target will be quietly removed.
										-- use this to kill an entity that you've already got a server-side ragdoll for

DMG_PHYSGUN =		(1<<23)		-- Hit by manipulator. Usually doesn't do any damage.
DMG_IGNITE =		(1<<24)		-- Damage caused by ignition
DMG_USE_HITLOCATIONS =	(1<<25)		-- For most hitscan guns: Headshot causes critical hits

DMG_DONT_COUNT_DAMAGE_TOWARDS_CRIT_RATE = (1<<26) -- Damage does not count towards crit rate
DMG_MELEE =	        (1<<27)		-- Melee damage
DMG_DIRECT =		(1<<28)
DMG_BUCKSHOT =		(1<<29)		-- not quite a bullet. Little, rounder, different.

DMG_FROM_OTHER_SAPPER = DMG_IGNITE

----------------
-- Custom damage types
----------------
TF_DMG_CUSTOM_NONE = 0
TF_DMG_CUSTOM_HEADSHOT = 1
TF_DMG_CUSTOM_BACKSTAB = 2
TF_DMG_CUSTOM_BURNING = 3
TF_DMG_WRENCH_FIX = 4
TF_DMG_CUSTOM_MINIGUN = 5
TF_DMG_CUSTOM_SUICIDE = 6
TF_DMG_CUSTOM_TAUNTATK_HADOUKEN = 7
TF_DMG_CUSTOM_BURNING_FLARE = 8
TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON = 9
TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM = 10
TF_DMG_CUSTOM_PENETRATE_MY_TEAM = 11
TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS = 12
TF_DMG_CUSTOM_TAUNTATK_FENCING = 13
TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE = 14
TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB = 15
TF_DMG_CUSTOM_TELEFRAG = 16
TF_DMG_CUSTOM_BURNING_ARROW = 17
TF_DMG_CUSTOM_FLYINGBURN = 18
TF_DMG_CUSTOM_PUMPKIN_BOMB = 19
TF_DMG_CUSTOM_DECAPITATION = 20
TF_DMG_CUSTOM_TAUNTATK_GRENADE = 21
TF_DMG_CUSTOM_BASEBALL = 22
TF_DMG_CUSTOM_CHARGE_IMPACT = 23
TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING = 24
TF_DMG_CUSTOM_AIR_STICKY_BURST = 25
TF_DMG_CUSTOM_DEFENSIVE_STICKY = 26
TF_DMG_CUSTOM_PICKAXE = 27
TF_DMG_CUSTOM_ROCKET_DIRECTHIT = 28
TF_DMG_CUSTOM_TAUNTATK_UBERSLICE = 29
TF_DMG_CUSTOM_PLAYER_SENTRY = 30
TF_DMG_CUSTOM_STANDARD_STICKY = 31
TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT = 32
TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH = 33
TF_DMG_CUSTOM_BLEEDING = 34
TF_DMG_CUSTOM_GOLD_WRENCH = 35
TF_DMG_CUSTOM_CARRIED_BUILDING = 36
TF_DMG_CUSTOM_COMBO_PUNCH = 37
TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL = 38
TF_DMG_CUSTOM_FISH_KILL = 39
TF_DMG_CUSTOM_TRIGGER_HURT = 40
TF_DMG_CUSTOM_DECAPITATION_BOSS = 41
TF_DMG_CUSTOM_STICKBOMB_EXPLOSION = 42
TF_DMG_CUSTOM_AEGIS_ROUND = 43
TF_DMG_CUSTOM_FLARE_EXPLOSION = 44
TF_DMG_CUSTOM_BOOTS_STOMP = 45
TF_DMG_CUSTOM_PLASMA = 46
TF_DMG_CUSTOM_PLASMA_CHARGED = 47
TF_DMG_CUSTOM_PLASMA_GIB = 48
TF_DMG_CUSTOM_PRACTICE_STICKY = 49
TF_DMG_CUSTOM_EYEBALL_ROCKET = 50
TF_DMG_CUSTOM_HEADSHOT_DECAPITATION = 51
TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON = 52
TF_DMG_CUSTOM_FLARE_PELLET = 53
TF_DMG_CUSTOM_CLEAVER = 54
TF_DMG_CUSTOM_CLEAVER_CRIT = 55
TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH = 56
TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB = 57
TF_DMG_CUSTOM_MERASMUS_GRENADE = 58
TF_DMG_CUSTOM_MERASMUS_ZAP = 59
TF_DMG_CUSTOM_MERASMUS_DECAPITATION = 60
TF_DMG_CUSTOM_CANNONBALL_PUSH = 61
TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF = 62
TF_DMG_CUSTOM_THROWABLE = 63
TF_DMG_CUSTOM_THROWABLE_KILL = 64
TF_DMG_CUSTOM_SPELL_TELEPORT = 65
TF_DMG_CUSTOM_SPELL_SKELETON = 66
TF_DMG_CUSTOM_SPELL_MIRV = 67
TF_DMG_CUSTOM_SPELL_METEOR = 68
TF_DMG_CUSTOM_SPELL_LIGHTNING = 69
TF_DMG_CUSTOM_SPELL_FIREBALL = 70
TF_DMG_CUSTOM_SPELL_MONOCULUS = 71
TF_DMG_CUSTOM_SPELL_BLASTJUMP = 72
TF_DMG_CUSTOM_SPELL_BATS = 73
TF_DMG_CUSTOM_SPELL_TINY = 74
TF_DMG_CUSTOM_KART = 75
TF_DMG_CUSTOM_GIANT_HAMMER = 76
TF_DMG_CUSTOM_RUNE_REFLECT = 77
TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE = 78
TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING = 79
TF_DMG_CUSTOM_SLAP_KILL = 80
TF_DMG_CUSTOM_CROC = 81
TF_DMG_CUSTOM_TAUNTATK_GASBLAST = 82

----------------
-- TF2 Classes
----------------
TF_CLASS_UNDEFINED = 0
TF_CLASS_SCOUT = 1
TF_CLASS_SNIPER = 2
TF_CLASS_SOLDIER = 3
TF_CLASS_DEMOMAN = 4
TF_CLASS_MEDIC = 5
TF_CLASS_HEAVYWEAPONS = 6
TF_CLASS_PYRO = 7
TF_CLASS_SPY = 8
TF_CLASS_ENGINEER = 9

----------------
-- AddCond conditions
----------------
TF_COND_INVALID                          =  -1
TF_COND_AIMING                           =   0
TF_COND_ZOOMED                           =   1
TF_COND_DISGUISING                       =   2
TF_COND_DISGUISED                        =   3
TF_COND_STEALTHED                        =   4
TF_COND_INVULNERABLE                     =   5
TF_COND_TELEPORTED                       =   6
TF_COND_TAUNTING                         =   7
TF_COND_INVULNERABLE_WEARINGOFF          =   8
TF_COND_STEALTHED_BLINK                  =   9
TF_COND_SELECTED_TO_TELEPORT             =  10
TF_COND_CRITBOOSTED                      =  11
TF_COND_TMPDAMAGEBONUS                   =  12
TF_COND_FEIGN_DEATH                      =  13
TF_COND_PHASE                            =  14
TF_COND_STUNNED                          =  15
TF_COND_OFFENSEBUFF                      =  16
TF_COND_SHIELD_CHARGE                    =  17
TF_COND_DEMO_BUFF                        =  18
TF_COND_ENERGY_BUFF                      =  19
TF_COND_RADIUSHEAL                       =  20
TF_COND_HEALTH_BUFF                      =  21
TF_COND_BURNING                          =  22
TF_COND_HEALTH_OVERHEALED                =  23
TF_COND_URINE                            =  24
TF_COND_BLEEDING                         =  25
TF_COND_DEFENSEBUFF                      =  26
TF_COND_MAD_MILK                         =  27
TF_COND_MEGAHEAL                         =  28
TF_COND_REGENONDAMAGEBUFF                =  29
TF_COND_MARKEDFORDEATH                   =  30
TF_COND_NOHEALINGDAMAGEBUFF              =  31
TF_COND_SPEED_BOOST                      =  32
TF_COND_CRITBOOSTED_PUMPKIN              =  33
TF_COND_CRITBOOSTED_USER_BUFF            =  34
TF_COND_CRITBOOSTED_DEMO_CHARGE          =  35
TF_COND_SODAPOPPER_HYPE                  =  36
TF_COND_CRITBOOSTED_FIRST_BLOOD          =  37
TF_COND_CRITBOOSTED_BONUS_TIME           =  38
TF_COND_CRITBOOSTED_CTF_CAPTURE          =  39
TF_COND_CRITBOOSTED_ON_KILL              =  40
TF_COND_CANNOT_SWITCH_FROM_MELEE         =  41
TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK        =  42
TF_COND_REPROGRAMMED                     =  43
TF_COND_CRITBOOSTED_RAGE_BUFF            =  44
TF_COND_DEFENSEBUFF_HIGH                 =  45
TF_COND_SNIPERCHARGE_RAGE_BUFF           =  46
TF_COND_DISGUISE_WEARINGOFF              =  47
TF_COND_MARKEDFORDEATH_SILENT            =  48
TF_COND_DISGUISED_AS_DISPENSER           =  49
TF_COND_SAPPED                           =  50
TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED =  51
TF_COND_INVULNERABLE_USER_BUFF           =  52
TF_COND_HALLOWEEN_BOMB_HEAD              =  53
TF_COND_HALLOWEEN_THRILLER               =  54
TF_COND_RADIUSHEAL_ON_DAMAGE             =  55
TF_COND_CRITBOOSTED_CARD_EFFECT          =  56
TF_COND_INVULNERABLE_CARD_EFFECT         =  57
TF_COND_MEDIGUN_UBER_BULLET_RESIST       =  58
TF_COND_MEDIGUN_UBER_BLAST_RESIST        =  59
TF_COND_MEDIGUN_UBER_FIRE_RESIST         =  60
TF_COND_MEDIGUN_SMALL_BULLET_RESIST      =  61
TF_COND_MEDIGUN_SMALL_BLAST_RESIST       =  62
TF_COND_MEDIGUN_SMALL_FIRE_RESIST        =  63
TF_COND_STEALTHED_USER_BUFF              =  64
TF_COND_MEDIGUN_DEBUFF                   =  65
TF_COND_STEALTHED_USER_BUFF_FADING       =  66
TF_COND_BULLET_IMMUNE                    =  67
TF_COND_BLAST_IMMUNE                     =  68
TF_COND_FIRE_IMMUNE                      =  69
TF_COND_PREVENT_DEATH                    =  70
TF_COND_MVM_BOT_STUN_RADIOWAVE           =  71
TF_COND_HALLOWEEN_SPEED_BOOST            =  72
TF_COND_HALLOWEEN_QUICK_HEAL             =  73
TF_COND_HALLOWEEN_GIANT                  =  74
TF_COND_HALLOWEEN_TINY                   =  75
TF_COND_HALLOWEEN_IN_HELL                =  76
TF_COND_HALLOWEEN_GHOST_MODE             =  77
TF_COND_MINICRITBOOSTED_ON_KILL          =  78
TF_COND_OBSCURED_SMOKE                   =  79
TF_COND_PARACHUTE_ACTIVE                 =  80
TF_COND_BLASTJUMPING                     =  81
TF_COND_HALLOWEEN_KART                   =  82
TF_COND_HALLOWEEN_KART_DASH              =  83
TF_COND_BALLOON_HEAD                     =  84
TF_COND_MELEE_ONLY                       =  85
TF_COND_SWIMMING_CURSE                   =  86
TF_COND_FREEZE_INPUT                     =  87
TF_COND_HALLOWEEN_KART_CAGE              =  88
TF_COND_DONOTUSE_0                       =  89
TF_COND_RUNE_STRENGTH                    =  90
TF_COND_RUNE_HASTE                       =  91
TF_COND_RUNE_REGEN                       =  92
TF_COND_RUNE_RESIST                      =  93
TF_COND_RUNE_VAMPIRE                     =  94
TF_COND_RUNE_REFLECT                     =  95
TF_COND_RUNE_PRECISION                   =  96
TF_COND_RUNE_AGILITY                     =  97
TF_COND_GRAPPLINGHOOK                    =  98
TF_COND_GRAPPLINGHOOK_SAFEFALL           =  99
TF_COND_GRAPPLINGHOOK_LATCHED            = 100
TF_COND_GRAPPLINGHOOK_BLEEDING           = 101
TF_COND_AFTERBURN_IMMUNE                 = 102
TF_COND_RUNE_KNOCKOUT                    = 103
TF_COND_RUNE_IMBALANCE                   = 104
TF_COND_CRITBOOSTED_RUNE_TEMP            = 105
TF_COND_PASSTIME_INTERCEPTION            = 106
TF_COND_SWIMMING_NO_EFFECTS              = 107
TF_COND_PURGATORY                        = 108
TF_COND_RUNE_KING                        = 109
TF_COND_RUNE_PLAGUE                      = 110
TF_COND_RUNE_SUPERNOVA                   = 111
TF_COND_PLAGUE                           = 112
TF_COND_KING_BUFFED                      = 113
TF_COND_TEAM_GLOWS                       = 114
TF_COND_KNOCKED_INTO_AIR                 = 115
TF_COND_COMPETITIVE_WINNER               = 116
TF_COND_COMPETITIVE_LOSER                = 117
TF_COND_HEALING_DEBUFF                   = 118
TF_COND_PASSTIME_PENALTY_DEBUFF          = 119
TF_COND_GRAPPLED_TO_PLAYER               = 120
TF_COND_GRAPPLED_BY_PLAYER               = 121
TF_COND_PARACHUTE_DEPLOYED               = 122
TF_COND_GAS                              = 123
TF_COND_BURNING_PYRO                     = 124
TF_COND_ROCKETPACK                       = 125
TF_COND_LOST_FOOTING                     = 126
TF_COND_AIR_CURRENT                      = 127
TF_COND_HALLOWEEN_HELL_HEAL              = 128
TF_COND_POWERUPMODE_DOMINANT             = 129

----------------
-- Stun types
----------------

TF_STUNFLAG_SLOWDOWN        = (1 << 0) --< activates slowdown modifier
TF_STUNFLAG_BONKSTUCK       = (1 << 1) --< bonk sound, stuck
TF_STUNFLAG_LIMITMOVEMENT   = (1 << 2) --< disable forward/backward movement
TF_STUNFLAG_CHEERSOUND      = (1 << 3) --< cheering sound
TF_STUNFLAG_NOSOUNDOREFFECT = (1 << 5) --< no sound or particle
TF_STUNFLAG_THIRDPERSON     = (1 << 6) --< panic animation
TF_STUNFLAG_GHOSTEFFECT     = (1 << 7) --< ghost particles

TF_STUNFLAGS_LOSERSTATE     = TF_STUNFLAG_SLOWDOWN | TF_STUNFLAG_NOSOUNDOREFFECT | TF_STUNFLAG_THIRDPERSON
TF_STUNFLAGS_GHOSTSCARE     = TF_STUNFLAG_GHOSTEFFECT | TF_STUNFLAG_THIRDPERSON
TF_STUNFLAGS_SMALLBONK      = TF_STUNFLAG_THIRDPERSON | TF_STUNFLAG_SLOWDOWN
TF_STUNFLAGS_NORMALBONK     = TF_STUNFLAG_BONKSTUCK
TF_STUNFLAGS_BIGBONK        = TF_STUNFLAG_CHEERSOUND | TF_STUNFLAG_BONKSTUCK

----------------
-- Input keys
----------------
IN_ATTACK		= (1 << 0)
IN_JUMP			= (1 << 1)
IN_DUCK			= (1 << 2)
IN_FORWARD		= (1 << 3)
IN_BACK			= (1 << 4)
IN_USE			= (1 << 5)
IN_CANCEL		= (1 << 6)
IN_LEFT			= (1 << 7)
IN_RIGHT		= (1 << 8)
IN_MOVELEFT		= (1 << 9)
IN_MOVERIGHT	= (1 << 10)
IN_ATTACK2		= (1 << 11)
IN_RUN			= (1 << 12)
IN_RELOAD		= (1 << 13)
IN_ALT1			= (1 << 14)
IN_ALT2			= (1 << 15)
IN_SCORE		= (1 << 16)   -- Used by client.dll for when scoreboard is held down
IN_SPEED		= (1 << 17)	-- Player is holding the speed key
IN_WALK			= (1 << 18)	-- Player holding walk key
IN_ZOOM			= (1 << 19)	-- Zoom key for HUD zoom
IN_WEAPON1		= (1 << 20)	-- weapon defines these bits
IN_WEAPON2		= (1 << 21)	-- weapon defines these bits
IN_BULLRUSH		= (1 << 22)
IN_GRENADE1		= (1 << 23)	-- grenade 1
IN_GRENADE2		= (1 << 24)	-- grenade 2
IN_ATTACK3		= (1 << 25)

----------------
-- Entity Effects 
----------------
EF_BONEMERGE			= 0x001	-- Performs bone merge on client side
EF_BRIGHTLIGHT 			= 0x002	-- DLIGHT centered at entity origin
EF_DIMLIGHT 			= 0x004	-- player flashlight
EF_NOINTERP				= 0x008	-- don't interpolate the next frame
EF_NOSHADOW				= 0x010	-- Don't cast no shadow
EF_NODRAW				= 0x020	-- don't draw entity
EF_NORECEIVESHADOW		= 0x040	-- Don't receive no shadow
EF_BONEMERGE_FASTCULL	= 0x080	-- For use with EF_BONEMERGE. If this is set then it places this ent's origin at its
                                    -- parent and uses the parent's bbox + the max extents of the aiment.
                                    -- Otherwise it sets up the parent's bones every frame to figure out where to place
                                    -- the aiment which is inefficient because it'll setup the parent's bones even if
                                    -- the parent is not in the PVS.
EF_ITEM_BLINK			= 0x100	-- blink an item so that the user notices it.
EF_PARENT_ANIMATES		= 0x200	-- always assume that the parent entity is animating

----------------
-- Entity Render Modes
----------------
RenderNormal = 0		-- src
RenderTransColor = 1		-- c*a+dest*= (1-a)
RenderTransTexture = 2	-- src*a+dest*= (1-a)
RenderGlow	= 3		-- src*a+dest -- No Z buffer checks -- Fixed size in screen space
RenderTransAlpha = 4		-- src*srca+dest*= (1-srca)
RenderTransAdd = 5		-- src*a+dest
RenderEnvironmental = 6	-- not drawn used for environmental effects
RenderTransAddFrameBlend = 7 -- use a fractional frame value to blend between animation frames
RenderTransAlphaAdd = 8	-- src + dest*= (1-a)
RenderWorldGlow = 9		-- Same as kRenderGlow but not fixed size in screen space
RenderNone = 10			-- Don't render.

----------------
-- Entity Render FX
----------------
RenderFxNone = 0 
RenderFxPulseSlow = 1
RenderFxPulseFast = 2
RenderFxPulseSlowWide = 3 
RenderFxPulseFastWide = 4
RenderFxFadeSlow = 5
RenderFxFadeFast = 6
RenderFxSolidSlow = 7
RenderFxSolidFast = 8 
RenderFxStrobeSlow = 9
RenderFxStrobeFast = 10
RenderFxStrobeFaster = 11
RenderFxFlickerSlow = 12
RenderFxFlickerFast = 13
RenderFxNoDissipation = 14
RenderFxDistort = 15			-- Distort/scale/translate flicker
RenderFxHologram = 16		-- kRenderFxDistort + distance fade
RenderFxExplode = 17			-- Scale up really big!
RenderFxGlowShell = 18		-- Glowing Shell
RenderFxClampMinScale = 19	-- Keep this sprite from getting very small = (SPRITES only!)
RenderFxEnvRain = 20		-- for environmental rendermode make rain
RenderFxEnvSnow = 21		--  "        "            "     make snow
RenderFxSpotlight = 22			-- TEST CODE for experimental spotlight
RenderFxRagdoll = 23			-- HACKHACK: TEST CODE for signalling death of a ragdoll character
RenderFxPulseFastWider = 24

----------------
-- Entity Solid Type
----------------
SOLID_NONE			= 0	-- no solid model
SOLID_BSP			= 1	-- a BSP tree
SOLID_BBOX			= 2	-- an AABB
SOLID_OBB			= 3	-- an OBB = (not implemented yet)
SOLID_OBB_YAW		= 4	-- an OBB constrained so that it can only yaw
SOLID_CUSTOM		= 5	-- Always call into the entity for tests
SOLID_VPHYSICS		= 6	-- solid vphysics object get vcollide from the model and collide with that

----------------
-- Entity Move Type
----------------
MOVETYPE_NONE		= 0	-- never moves
MOVETYPE_ISOMETRIC = 1			-- For players -- in TF2 commander view etc.
MOVETYPE_WALK = 2				-- Player only - moving on the ground
MOVETYPE_STEP = 3				-- gravity special edge handling -- monsters use this
MOVETYPE_FLY = 4				-- No gravity but still collides with stuff
MOVETYPE_FLYGRAVITY = 5		-- flies through the air + is affected by gravity
MOVETYPE_VPHYSICS = 6			-- uses VPHYSICS for simulation
MOVETYPE_PUSH = 7				-- no clip to world push and crush
MOVETYPE_NOCLIP = 8			-- No gravity no collisions still do velocity/avelocity
MOVETYPE_LADDER = 9			-- Used by players only when going onto a ladder
MOVETYPE_OBSERVER = 10			-- Observer movement depends on player's observer mode
MOVETYPE_CUSTOM = 11			-- Allows the entity to describe its own physics

----------------
-- Entity Flags m_fFlags
----------------
FL_ONGROUND				= (1<<0)	-- At rest / on the ground
FL_DUCKING				= (1<<1)	-- Player flag -- Player is fully crouched
FL_ANIMDUCKING			= (1<<2)	-- Player flag -- Player is in the process of crouching or uncrouching but could be in transition
-- examples:                                   Fully ducked:  FL_DUCKING &  FL_ANIMDUCKING
--           Previously fully ducked unducking in progress:  FL_DUCKING & !FL_ANIMDUCKING
--                                           Fully unducked: !FL_DUCKING & !FL_ANIMDUCKING
--           Previously fully unducked ducking in progress: !FL_DUCKING &  FL_ANIMDUCKING
FL_WATERJUMP			= (1<<3)	-- player jumping out of water
FL_ONTRAIN				= (1<<4) -- Player is _controlling_ a train so movement commands should be ignored on client during prediction.
FL_INRAIN				= (1<<5)	-- Indicates the entity is standing in rain
FL_FROZEN				= (1<<6) -- Player is frozen for 3rd person camera
FL_ATCONTROLS			= (1<<7) -- Player can't move but keeps key inputs for controlling another entity
FL_CLIENT				= (1<<8)	-- Is a player
FL_FAKECLIENT			= (1<<9)	-- Fake client simulated server side; don't send network messages to them
-- NON-PLAYER SPECIFIC = (i.e. not used by GameMovement or the client .dll ) -- Can still be applied to players though
FL_INWATER				= (1<<10)	-- In water

FL_FLY					= (1<<11)	-- Changes the SV_Movestep= () behavior to not need to be on ground
FL_SWIM					= (1<<12)	-- Changes the SV_Movestep= () behavior to not need to be on ground = (but stay in water)
FL_CONVEYOR				= (1<<13)
FL_NPC					= (1<<14)
FL_GODMODE				= (1<<15)
FL_NOTARGET				= (1<<16)
FL_AIMTARGET			= (1<<17)	-- set if the crosshair needs to aim onto the entity
FL_PARTIALGROUND		= (1<<18)	-- not all corners are valid
FL_STATICPROP			= (1<<19)	-- Eetsa static prop!		
FL_GRAPHED				= (1<<20) -- worldgraph has this ent listed as something that blocks a connection
FL_GRENADE				= (1<<21)
FL_STEPMOVEMENT			= (1<<22)	-- Changes the SV_Movestep= () behavior to not do any processing
FL_DONTTOUCH			= (1<<23)	-- Doesn't generate touch functions generates Untouch= () for anything it was touching when this flag was set
FL_BASEVELOCITY			= (1<<24)	-- Base velocity has been applied this frame = (used to convert base velocity into momentum)
FL_WORLDBRUSH			= (1<<25)	-- Not moveable/removeable brush entity = (really part of the world but represented as an entity for transparency or something)
FL_OBJECT				= (1<<26) -- Terrible name. This is an object that NPCs should see. Missiles for example.
FL_KILLME				= (1<<27)	-- This entity is marked for death -- will be freed by game DLL
FL_ONFIRE				= (1<<28)	-- You know...
FL_DISSOLVING			= (1<<29) -- We're dissolving!
FL_TRANSRAGDOLL			= (1<<30) -- In the process of turning into a client side ragdoll.
FL_UNBLOCKABLE_BY_PLAYER = (1<<31) -- pusher that can't be blocked by the player

LOADOUT_POSITION_PRIMARY = 0
LOADOUT_POSITION_SECONDARY = 1
LOADOUT_POSITION_MELEE = 2
LOADOUT_POSITION_UTILITY = 3
LOADOUT_POSITION_BUILDING = 4
LOADOUT_POSITION_PDA = 5
LOADOUT_POSITION_PDA2 = 6

-- wearables
LOADOUT_POSITION_HEAD = 7
LOADOUT_POSITION_MISC = 8

-- other
LOADOUT_POSITION_ACTION = 9

LOADOUT_POSITION_MISC2 = 10

-- taunts
LOADOUT_POSITION_TAUNT = 11
LOADOUT_POSITION_TAUNT2 = 12
LOADOUT_POSITION_TAUNT3 = 13
LOADOUT_POSITION_TAUNT4 = 14
LOADOUT_POSITION_TAUNT5 = 15
LOADOUT_POSITION_TAUNT6 = 16
LOADOUT_POSITION_TAUNT7 = 17
LOADOUT_POSITION_TAUNT8 = 18

----------------
-- Entity callbacks
----------------
ON_REMOVE = 0               -- Callback function parameters: entity
ON_SPAWN = 1                -- Callback function parameters: entity
ON_ACTIVATE = 2             -- Callback function parameters: entity
ON_DAMAGE_RECEIVED_PRE = 3  -- Callback function parameters: entity, damageinfo. Return true to apply changes made in damageinfo table 
ON_DAMAGE_RECEIVED_POST = 4 -- Callback function parameters: entity, damageinfo, previousHealth
ON_INPUT = 5                -- Callback function parameters: entity, inputName, value, activator, caller. Return true to stop entity from processing the input
ON_OUTPUT = 6               -- Callback function parameters: entity, outputName, value, activator. Return true to stop entity from processing the output
ON_KEY_PRESSED = 7          -- Player only callback. Look at IN_* globals for more info. Callback function parameters: entity, key
ON_KEY_RELEASED = 8         -- Player only callback. Look at IN_* globals for more info. Callback function parameters: entity, key
ON_DEATH = 9                -- Callback function parameters: entity
ON_EQUIP_ITEM = 10          -- Player only callback. Callback function parameters: entity, weapon. Return false to prevent equipping the weapon
ON_DEPLOY_WEAPON = 11       -- Player only callback. Callback function parameters: entity, weapon. Return false to stop deploying the weapon
ON_PROVIDE_ITEMS = 12       -- Player only callback. Called when loadout items are provided to the player. Callback function parameters: entity
ON_TOUCH = 13               -- Called every tick the entity is touched. Callback function parameters: entity, other, hitPos, hitNormal
ON_START_TOUCH = 14         -- Callback function parameters: entity, other, hitPos, hitNormal
ON_END_TOUCH = 15           -- Callback function parameters: entity, other, hitPos, hitNormal
ON_SHOULD_COLLIDE = 16      -- Callback function parameters: entity, other, cause. Return false to disable collision, true to enable
ON_HOLSTER_WEAPON = 17      -- Player only callback. Callback function parameters: entity, oldWeapon, newWeapon. Return false to stop holstering the weapon
ON_FIRE_WEAPON_PRE = 18     -- Weapon only callback. Callback function parameters: entity. Return ACTION_STOP to prevent weapon from firing, ACTION_HANDLED to stop projectile from being fired but still consume ammo
ON_FIRE_WEAPON_POST = 19    -- Weapon only callback. Callback function parameters: entity, projectile.

----------------
-- Print targets
----------------
PRINT_TARGET_CONSOLE = 0
PRINT_TARGET_CHAT = 1
PRINT_TARGET_CENTER = 2
PRINT_TARGET_HINT = 3 -- Message on the bottom part of the screen, plays a cue sound
PRINT_TARGET_RIGHT = 4 -- Message on the right part of the screen

----------------
-- Menu flags
----------------
MENUFLAG_BUTTON_EXIT	    = (1<<0) -- Menu has an "exit" button
MENUFLAG_BUTTON_EXITBACK	= (1<<1) -- Menu has an "exit back" button
MENUFLAG_NO_SOUND           = (1<<2) -- Menu will not have any select sounds

----------------
-- Event callback actions
----------------
ACTION_CONTINUE	            = 0 -- Continue event as usual with no changes
ACTION_STOP                 = 1 -- Stop event from being performed
ACTION_MODIFY               = 2 -- Continue event with modified values
ACTION_HANDLED              = 3 -- Stop original event, but pretend it happened

----------------
-- Return values for GetWaveSpawnLocation
----------------
SPAWN_LOCATION_NOT_FOUND  = 0 -- Do not spawn bots at this location
SPAWN_LOCATION_NAV        = 1 -- Spawn bots at nav area center below position specified by 2nd vector return value
SPAWN_LOCATION_TELEPORTER = 2 -- Spawn bots at position specified by 2nd vector return value, and to teleport effects

----------------
-- Causes for ON_SHOULD_COLLIDE
----------------
ON_SHOULD_COLLIDE_CAUSE_FIRE_WEAPON = 0 -- Caused by firing weapons
ON_SHOULD_COLLIDE_CAUSE_OTHER       = 1 -- Caused from any other source

----------------
-- Table structures
----------------

---@class ShowHUDTextParams
---@field channel number = 4
---@field x number = -1. X coordinate from 0 to 1. -1 for center positon. Text is wrapped so it does not overflow the screen
---@field y number = -1. Y coordinate from 0 to 1. -1 for center positonText is wrapped so it does not overflow the screen
---@field effect number = 0. 0,1 - fade in/fade out text. 2 - typeout text
---@field r1 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field r2 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field g1 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field g2 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field b1 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field b2 number = 255. 0 - 255 range. The text is rendered additively, black text will not display
---@field a1 number = 0. 0 - 255 range. 0 - fully visible, 255 - invisible
---@field a2 number = 0. 0 - 255 range. 0 - fully visible, 255 - invisible
---@field fadeinTime number = 0. Time to fade in text
---@field fadeoutTime number = 0. Time to fade out text
---@field holdTime number = 9999. Time the text is fully displayed
---@field fxTime number = 0. Time to type a single letter with typeout effect
DefaultHudTextParams = {
    channel = 4, --Channel number 0 - 5, -- Channel 0 is used to display wave explanation, -- Channel 1 is used to display cash in reverse mvm,
    x = -1, -- X coordinate from 0 to 1, -- -1 for center positon, -- Text is wrapped so it does not overflow the screen
    y = -1, -- Y coordinate from 0 to 1, -- -1 for center positonText is wrapped so it does not overflow the screen
    effect = 0, -- 0,1 - fade in/fade out text, -- 2 - typeout text
    r1 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    r2 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    g1 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    g2 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    b1 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    b2 = 255, -- 0 - 255 range, -- The text is rendered additively, black text will not display
    a1 = 0, -- 0 - 255 range, -- 0 - fully visible, 255 - invisible
    a2 = 0, -- 0 - 255 range, -- 0 - fully visible, 255 - invisible
    fadeinTime = 0, -- Time to fade in text
    fadeoutTime = 0, -- Time to fade out text
    holdTime = 9999, -- Time the text is fully displayed
    fxTime = 0, -- Time to type a single letter with typeout effect
}

---@class TraceInfo
---@field start Vector|Entity
---@field endpos Vector|nil
---@field distance number
---@field angles Vector
---@field mask number
---@field collisiongroup number
---@field mins Vector
---@field maxs Vector
---@field filter function|table|Entity|nil
DefaultTraceInfo = {
    start = Vector(0,0,0), -- Start position vector. Can also be set to entity, in this case the trace will start from entity eyes position
    endpos = nil, -- End position vector. If nil, the trace will be fired in `angles` direction with `distance` length
    distance = 8192, -- Used if endpos is nil
    angles = Vector(0,0,0), -- Used if endpos is nil
    mask = MASK_SOLID, -- Solid type mask, see MASK_* globals
    collisiongroup = COLLISION_GROUP_NONE, -- Pretend the trace to be fired by an entity belonging to this group. See COLLISION_GROUP_* globals
    mins = Vector(0,0,0), -- Extends the size of the trace in negative direction
    maxs = Vector(0,0,0), -- Extends the size of the trace in positive direction
    filter = nil -- Entity to ignore. If nil, filters start entity. Can be a single entity, table of entities, or a function with a single entity parameter that returns true if trace should hit the entity, false otherwise.
}

---@class TraceResultInfo
---@field Entity Entity|nil
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
DefaultTraceResultInfo = {
    Entity = nil, -- The hit entity
    Fraction = 1, -- Distance to the target divided by trace length. 1 if did not hit anything
    FractionLeftSolid = 0,
    Hit = false, -- `true` if trace hit anything
    HitBox = 0, -- Hitbox of the target
    HitGroup = HITGROUP_GENERIC, -- Hit group of the target, see HITGROUP_* globals
    HitNoDraw = false, -- `true` if nodraw brush was hit
    HitNonWorld = false, -- `true` if the target was not a static brush or prop
    HitNormal = Vector(0,0,0), -- Normal of the surface being hit
    HitPos = Vector(0,0,0), -- Position where the trace hit
    HitSky = false, -- `true` if skybox was hit
    HitTexture = "**empty**", -- Material of the hit brush. **displacement** is returned for displacements and **studio** for props
    HitWorld = false, -- `true` if the target was the world
    Normal = Vector(0,0,0), -- Normalized direction of the trace
    StartPos = Vector(0,0,0), -- Starting point of the trace
    StartSolid = false, -- `true` if the trace started inside solid
    SurfaceFlags = 0, -- See SURF_* globals
    DispFlags = 0, -- See DISPSURF_* globals
    Contents = 0 -- See CONTENTS_* globals
}

---@class TakeDamageInfo
---@field Attacker Entity|nil
---@field Inflictor Entity|nil
---@field Weapon Entity|nil
---@field Damage number
---@field DamageType number
---@field DamageCustom number
---@field CritType number
---@field DamagePosition Vector
---@field DamageForce Vector
---@field ReportedPosition Vector
DefaultTakeDamageInfo = {
    Attacker = nil, -- Attacker
    Inflictor = nil, -- Direct cause of damage, usually a projectile
    Weapon = nil,
    Damage = 0,
    DamageType = DMG_GENERIC, -- Damage type, see DMG_* globals. Can be combined with | operator
    DamageCustom = TF_DMG_CUSTOM_NONE, -- Custom damage type, see TF_DMG_* globals
    DamagePosition = Vector(0,0,0), -- Where the target was hit at
    DamageForce = Vector(0,0,0), -- Knockback force of the attack
    ReportedPosition = Vector(0,0,0) -- Where the attacker attacked from
}

-- A single menu entry
---@class MenuEntry
---@field text string
---@field value string
---@field disabled boolean
DefaultMenuEntry = {
    text = "", -- Text for display
    value = "", -- Internal value used in onSelect function
    disabled = false -- This entry cannot be selected
}

-- To add a new entry, add a number indexed key, with either string or MenuEntry table as a value
-- Example:
-- ```
-- {
--     1 = "simple",
--     2 = {text = "complex", value = "internal", disabled = true}
--     timeout = 0, -- How long to display the menu, or 0 for no timeout
--     title = "Menu",
--     itemsPerPage = nil,
--     flags = MENUFLAG_BUTTON_EXIT,
--     onSelect = function (player, index, value) end , -- Function called on select with parameters: player, index, value
--     onCancel = function (player, reason) end, -- Function called on cancel with parameters: player, reason
-- }
-- ```
---@class Menu
---@field timeout number
---@field title string
---@field itemsPerPage number|nil
---@field flags number
---@field onSelect function|nil
---@field onCancel function|nil
DefaultMenu = {
    timeout = 0, -- How long to display the menu, or 0 for no timeout
    title = "Menu",
    itemsPerPage = nil, -- How many items per page, nil for default (7). Can be set up to 10 for single page menu
    flags = MENUFLAG_BUTTON_EXIT,
    onSelect = nil, -- Function called on select with parameters: player, selectedIndex, value
    onCancel = nil, -- Function called on cancel with parameters: player, reason
}

-- SpawnTemplate info
---@class SpawnTemplateInfo
---@field translation Vector
---@field rotation Vector
---@field parent Entity|nil
---@field autoParent boolean
---@field attachment string|nil
---@field ignoreParentAliveState boolean
---@field params table|nil
DefaultSpawnTemplateInfo = {
    translation = Vector(0,0,0), -- Spawn position offset
    rotation = Vector(0,0,0), -- Spawn rotation angle
    parent = nil, -- Template parent. If set, template entities are spawned relatively to the parent, and are automatically removed when the parent dies
    autoParent = true, -- Automatically set parent of template entities without parentname
    attachment = nil, -- If set, parent template entities to this parent attachment
    ignoreParentAliveState = false, -- If true, only remove this template when the parent is removed, but not when it is killed
    params = nil, -- Table with parameters passed to the template. Example { paramname = "value" }
}

-- Returns an array with merged values from input tables
---@vararg table 
---@return table result
function table.JoinArray(...)
    local result = {}
    for i = 1, #arg do
        local argtable = arg[i]
        for k, v in pairs(argtable) do
            table.insert(result, v)
        end
    end 
    return result;
end

-- Returns a table with merged contents from input tables
---@vararg table 
---@return table result
function table.JoinTable(...)
    local result = {}
    for i = 1, #arg do
        local argtable = arg[i]
        for k, v in pairs(argtable) do
            result[k] = v
        end
    end 
    return result;
end

function table.ForEach(tab, funcname)

	for k, v in pairs(tab) do
		funcname(k, v)
	end

end

function table.HasValue(t, val)
	for k, v in pairs(t) do
		if (v == val) then return true end
	end
	return false
end

function table.Random(t)
	local rk = math.random(1, table.Count(t))
	local i = 1
	for k, v in pairs(t) do
		if (i == rk) then return v, k end
		i = i + 1
	end
end

function table.Count(t)
	local i = 0
	for k in pairs(t) do i = i + 1 end
	return i
end

-- Returns an array containing keys from a table
function table.GetKeys(tab)
	local keys = {}
	local id = 1

	for k, v in pairs(tab) do
		keys[id] = k
		id = id + 1
	end

	return keys
end

-- Return a deep copy of the table, also copying the nested tables. Does not copy metatables
---@param tab table The table to copy
function table.DeepCopy(tab)
	return table._DeepCopy(tab, {})
end

---@param tab table The table to copy
---@param done? table Ignore this parameter
function table._DeepCopy(tab, done)
    done = done or {}
	local result = {}

	for k, v in pairs(tab) do
		if type(v) == "table" then
            if not done[v] then
                result[k] = table.DeepCopy(v, done)
                done[v] = result[k]
            else
                result[k] = done[v]
            end
        else
            result[k] = v
        end
	end

	return result
end

-- Return a shallow copy of the table. Copies references of nested tables rather than their contents
---@param tab table The table to copy
function table.ShallowCopy(tab)
	local result = {}

	for k, v in pairs(tab) do
        result[k] = v
	end

	return result
end

--Prints an array of strings, should be used as a function inside timer.Create
function PrintDelay(t)
    --Initialize print index
    t.printIndex = t.printIndex or 1;

    for i= 0, 5 do
        print(t[t.printIndex])
        t.printIndex = t.printIndex + 1;
        if t.printIndex > #t then
            return false
        end
    end
end

--Prints table to console. The optional paramaters should be ignored
---@param t table The table to print
---@param indent? number
---@param done? table
---@param output? table
function PrintTable(t, indent, done, output)
    done = done or {}
    if output == nil then
        output = output or {}
        timer.Create(0, PrintDelay, 0, output);
    end
    indent = indent or 0
    local keys = table.GetKeys(t)
    local keysize = 0;
    table.sort(keys, function(a, b)
        if (type(a) == "number" and type(b) == "number") then return a < b end
        
        return tostring(a) < tostring(b)
    end)

    for k, v in ipairs(keys) do
        keysize = math.max(keysize, #tostring(v));
    end

    done[t] = true

    for i = 1, #keys do
        local key = keys[i]
        local value = t[key]
        local line = string.rep("\t", indent)

        if  (type(value) == "table" and not done[value] ) then
            done[value] = true
            table.insert(output, line..key..":");
            PrintTable(value, indent + 1, done, output)
            done[value] = nil
        else
            table.insert(output, line..key..string.rep(" ", keysize - #tostring(key) + 1).."=\t"..tostring(value))
        end
    end
end