#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfentities.h"
#include "stub/tfplayer.h"
#include "stub/extraentitydata.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "mod/pop/popmgr_extensions.h"


namespace Mod::Etc::Unintended_Class_Weapon_Improvements
{

	
	class UnintendedClassViewmodelOverride : public EntityModule
	{
	public:
		UnintendedClassViewmodelOverride(CBaseEntity *entity) : EntityModule(entity) {};

		CHandle<CBaseEntity> wearable;
		CHandle<CBaseEntity> wearableWeapon;
		int oldModelIndex;
		const char *properHandModel;
		int properHandModelIndex;
	};

    THINK_FUNC_DECL(HideViewModel)
	{
		this->AddEffects(EF_NODRAW);
		auto weapon = ToTFPlayer(reinterpret_cast<CTFViewModel *>(this)->GetOwner())->GetActiveTFWeapon();
		if (weapon != nullptr) {
			auto mod = weapon->GetEntityModule<UnintendedClassViewmodelOverride>("unintendedclassweapon");
			if (mod != nullptr) {
				weapon->SetModel(mod->properHandModel);
				weapon->m_iViewModelIndex = mod->properHandModelIndex;
			}
		}
	}

	bool OnEquipUnintendedClassWeapon(CTFPlayer *owner, CTFWeaponBase *weapon, UnintendedClassViewmodelOverride *mod) 
	{
		if (weapon->m_nCustomViewmodelModelIndex != mod->properHandModelIndex) return false;
		weapon->SetModel(mod->properHandModel);
		weapon->m_iViewModelIndex = mod->properHandModelIndex;
		auto wearable_vm = static_cast<CTFWearable *>(CreateEntityByName("tf_wearable_vm"));
		wearable_vm->Spawn();
		wearable_vm->GiveTo(owner);
		wearable_vm->SetModelIndex(mod->oldModelIndex);
		wearable_vm->m_bValidatedAttachedEntity = true;
		mod->wearable = wearable_vm;

        // Don't create wearable imitating weapon model if a custom model is being used
        if (!weapon->m_bBeingRepurposedForTaunt) {
            auto wearable_vm_weapon = static_cast<CTFWearable *>(CreateEntityByName("tf_wearable_vm"));
            wearable_vm_weapon->Spawn();
            wearable_vm_weapon->GiveTo(owner);
            wearable_vm_weapon->SetModelIndex(weapon->m_iWorldModelIndex);
            wearable_vm_weapon->m_bValidatedAttachedEntity = true;
            mod->wearableWeapon = wearable_vm_weapon;
        }
		owner->GetViewModel()->AddEffects(EF_NODRAW);
		THINK_FUNC_SET(owner->GetViewModel(), HideViewModel, gpGlobals->curtime);
		return true;
	}

	bool OnUnequipUnintendedClassWeapon(CTFPlayer *owner, CTFWeaponBase *weapon, UnintendedClassViewmodelOverride *mod) 
	{
		if (mod->wearable != nullptr) {
			mod->wearable->Remove();
		}
		if (mod->wearableWeapon != nullptr) {
			mod->wearableWeapon->Remove();
		}
		return true;
	}

    ConVar cvar_enable_viewmodel("sig_etc_unintended_class_weapon_viewmodel", "0", FCVAR_NOTIFY,
		"Mod: use proper class viewmodel animations for unintended player class weapons");

    DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_Equip, CBaseCombatCharacter *owner)
	{
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_Equip)(owner);
		
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
		
		auto weapon = rtti_cast<CTFWeaponBase *>(ent);
		int otherClassViewmodel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, otherClassViewmodel, use_original_class_weapon_animations);

		if (weapon != nullptr && (cvar_enable_viewmodel.GetBool() || otherClassViewmodel != 0) && weapon->GetItem() != nullptr) {
			
			// Use viewmodel of a real class if applicable
			auto owner = weapon->GetTFPlayerOwner();
			if (owner != nullptr && owner->IsRealPlayer() && owner->GetViewModel() != nullptr && weapon->m_nViewModelIndex == 0) {
				auto def = weapon->GetItem()->GetItemDefinition();
				auto classIndex = owner->GetPlayerClass()->GetClassIndex();
				bool goodAnims = (FStrEq(weapon->GetClassname(), "tf_weapon_handgun_scout_secondary") && classIndex == TF_CLASS_ENGINEER) 
					|| ((FStrEq(weapon->GetClassname(), "tf_weapon_shotgun_hwg") || FStrEq(weapon->GetClassname(), "tf_weapon_shotgun_soldier") || FStrEq(weapon->GetClassname(), "tf_weapon_shotgun_pyro")) 
					&& (classIndex == TF_CLASS_SOLDIER || classIndex == TF_CLASS_HEAVYWEAPONS || classIndex == TF_CLASS_PYRO));
#ifndef NO_MVM
                Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(true);
#endif
				if (!goodAnims && def->GetLoadoutSlot(classIndex) == -1) {
					int properClass = -1;
					for (int i = 1; i < 10; i++) {
						if (def->GetLoadoutSlot(i) != -1) {
							properClass = i;
							break;
						}
					}
					if (properClass != -1) {
						auto mod = weapon->GetOrCreateEntityModule<UnintendedClassViewmodelOverride>("unintendedclassweapon");
						mod->oldModelIndex = CBaseEntity::PrecacheModel(weapon->GetViewModel());
						mod->properHandModel = GetPlayerClassData(properClass)->m_szHandModelName;
						mod->properHandModelIndex = CBaseEntity::PrecacheModel(mod->properHandModel);
						weapon->SetCustomViewModel(mod->properHandModel);
						weapon->m_iViewModelIndex = mod->properHandModelIndex;
						weapon->SetModel(mod->properHandModel);
						if (weapon == owner->GetActiveTFWeapon()) {
							OnEquipUnintendedClassWeapon(owner, weapon, mod);
						}
					}
				}
#ifndef NO_MVM
                Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(false);
#endif
			}
		}
	} 

    DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Deploy)
	{
		auto wep = reinterpret_cast<CTFWeaponBase *>(this);
		auto mod = wep->GetEntityModule<UnintendedClassViewmodelOverride>("unintendedclassweapon");
		if (mod != nullptr && wep->GetTFPlayerOwner() != nullptr) {
			OnEquipUnintendedClassWeapon(wep->GetTFPlayerOwner(), wep, mod);
		}
		return DETOUR_MEMBER_CALL(CTFWeaponBase_Deploy)();
	}

    DETOUR_DECL_MEMBER(bool, CTFWeaponBase_Holster)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
        
		auto mod = weapon->GetEntityModule<UnintendedClassViewmodelOverride>("unintendedclassweapon");
		if (mod != nullptr && weapon->GetTFPlayerOwner() != nullptr) {
			OnUnequipUnintendedClassWeapon(weapon->GetTFPlayerOwner(), weapon, mod);
		}
        return DETOUR_MEMBER_CALL(CTFWeaponBase_Holster)();
    }

    DETOUR_DECL_MEMBER(void, CEconEntity_UpdateOnRemove)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		auto mod = entity->GetEntityModule<UnintendedClassViewmodelOverride>("unintendedclassweapon");
		if (mod != nullptr) {
			auto weapon = rtti_cast<CTFWeaponBase *>(entity);
			if (weapon != nullptr && weapon->GetTFPlayerOwner() != nullptr) {
				OnUnequipUnintendedClassWeapon(weapon->GetTFPlayerOwner(), weapon, mod);
			}
		}
        DETOUR_MEMBER_CALL(CEconEntity_UpdateOnRemove)();
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_Taunt, taunts_t index, int taunt_concept)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_Taunt)(index, taunt_concept);
        int playerClass = player->GetPlayerClass()->GetClassIndex();
        if (!player->m_Shared->InCond(TF_COND_TAUNTING) && player->GetActiveTFWeapon()->GetItem()->GetItemDefinition()->GetLoadoutSlot(playerClass) == -1) {
            int validClass = -1;
            for (int i = 1; i < 10; i++) {
                if (player->GetActiveTFWeapon()->GetItem()->GetItemDefinition()->GetLoadoutSlot(i) != -1) {
                    validClass = i;
                    break;
                }
            }
            if (validClass != -1) {
                auto oldClass = player->GetPlayerClass()->GetClassIndex();
                player->GetPlayerClass()->SetClassIndex(validClass);
                DETOUR_MEMBER_CALL(CTFPlayer_Taunt)(index, taunt_concept);
                player->GetPlayerClass()->SetClassIndex(oldClass);
            }
            // for (int i = 0; i < 48; i++) {
            //     auto weapon = player->GetWeapon(i);
            //     if (weapon != nullptr && weapon->GetItem()->GetItemDefinition()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex()) != -1) {
            //         auto oldActiveWeapon = player->GetActiveTFWeapon();
            //         player->SetActiveWeapon(weapon);
            //         Msg("This\n");
		    //         DETOUR_MEMBER_CALL(CTFPlayer_Taunt)(index, taunt_concept);
            //         player->SetActiveWeapon(oldActiveWeapon);
            //         break;
            //     }
            // }
        }
	}


    
    bool ActivateShield(CTFPlayer *player)
	{
		for (int i = 0; i < player->GetNumWearables(); i++) {
			auto shield = rtti_cast<CTFWearableDemoShield *>(player->GetWearable(i));
			if (shield != nullptr && !shield->m_bDisguiseWearable) {
				shield->DoSpecialAction(player);
				return true;
			}
		}
		return false; 
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_DoClassSpecialSkill)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_DoClassSpecialSkill)();

		if (!player->IsAlive())
			return result;

		if (player->m_Shared->InCond(TF_COND_HALLOWEEN_KART))
			return result;

		if (player->m_Shared->m_bHasPasstimeBall)
			return result; 

		if (!result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sticky = rtti_cast<CTFPipebombLauncher *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_SECONDARY));
			if (sticky != nullptr) {
				sticky->SecondaryAttack();
				result = true;
			}
			else {
				result = ActivateShield(player);
			}
		}

		if (!result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER && player->GetObjectCount() > 0 && player->Weapon_OwnsThisID(TF_WEAPON_BUILDER) != nullptr) {
			result = player->TryToPickupBuilding();
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_UpdateChargeMeter)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this);

		auto playerClass = player->GetOuter()->GetPlayerClass();
		int classPre = playerClass->GetClassIndex();
		playerClass->SetClassIndex(TF_CLASS_DEMOMAN);
		DETOUR_MEMBER_CALL(CTFPlayerShared_UpdateChargeMeter)();
		playerClass->SetClassIndex(classPre);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_EndClassSpecialSkill)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_EndClassSpecialSkill)();

		if (result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			int classPre = player->GetPlayerClass()->GetClassIndex();
			player->GetPlayerClass()->SetClassIndex(TF_CLASS_DEMOMAN);
			result = DETOUR_MEMBER_CALL(CTFPlayer_EndClassSpecialSkill)();
			player->GetPlayerClass()->SetClassIndex(classPre);
		}
		return result;
	}

	DETOUR_DECL_MEMBER(float, CTFPlayer_TeamFortress_CalculateMaxSpeed, bool flag)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		int classPre = player->GetPlayerClass()->GetClassIndex();
		bool charging = !flag && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN && player->m_Shared->InCond(TF_COND_SHIELD_CHARGE);
		if (charging) {
			player->GetPlayerClass()->SetClassIndex(TF_CLASS_DEMOMAN);
		}
		float ret = DETOUR_MEMBER_CALL(CTFPlayer_TeamFortress_CalculateMaxSpeed)(flag);
		
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sword = rtti_cast<CTFSword *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_MELEE));
			if (sword != nullptr) {
				ret *= sword->GetSwordSpeedMod();
			}
		}
		if (charging) {
			player->GetPlayerClass()->SetClassIndex(classPre);
		}

		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxHealthForBuffing)
	{
		int ret = DETOUR_MEMBER_CALL(CTFPlayer_GetMaxHealthForBuffing)();
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sword = rtti_cast<CTFSword *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_MELEE));
			if (sword != nullptr) {
				ret += sword->GetSwordHealthMod();
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CTFMinigun_SecondaryAttack)
	{
		auto player = reinterpret_cast<CTFMinigun *>(this)->GetTFPlayerOwner();
		DETOUR_MEMBER_CALL(CTFMinigun_SecondaryAttack)();
		ActivateShield(player);	
	}

	DETOUR_DECL_MEMBER(void, CTFFists_SecondaryAttack)
	{
		auto player = reinterpret_cast<CTFWeaponBaseMelee *>(this)->GetTFPlayerOwner();
		if (!ActivateShield(player)) {
			DETOUR_MEMBER_CALL(CTFFists_SecondaryAttack)();
		}
	}

	DETOUR_DECL_MEMBER(void, CTFFists_Punch)
	{
		auto player = reinterpret_cast<CTFWeaponBaseMelee *>(this)->GetTFPlayerOwner();
		for (int i = 0; i < player->GetNumWearables(); i++) {
			auto shield = rtti_cast<CTFWearableDemoShield *>(player->GetWearable(i));
			if (shield != nullptr) {
				shield->EndSpecialAction(player);
				break;
			}
		}
		DETOUR_MEMBER_CALL(CTFFists_Punch)();
	}

	DETOUR_DECL_STATIC(bool, ClassCanBuild, int classIndex, int type)
	{
		return true;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBuilder_SetSubType, int type)
	{
		if ((type < 0 || type >= OBJ_LAST) && type != 255)
			return;
		DETOUR_MEMBER_CALL(CTFWeaponBuilder_SetSubType)(type);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_ManageBuilderWeapons, TFPlayerClassData_t *pData)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int classIndex = player->GetPlayerClass()->GetClassIndex();
		// Do not manage builder weapons for other classes than engineer or spy. This way, custom builder items assigned to those classes are not being destroyed
		if (classIndex != TF_CLASS_ENGINEER && classIndex != TF_CLASS_SPY)
			return;

		DETOUR_MEMBER_CALL(CTFPlayer_ManageBuilderWeapons)(pData);
	}

	THINK_FUNC_DECL(SetTypeToSapper)
	{
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);
		if (builder != builder->GetTFPlayerOwner()->GetActiveTFWeapon()) {
			builder->SetSubType(3);
			builder->m_iObjectMode = 0;
		}
		builder->SetNextThink(gpGlobals->curtime + 0.5f, "SetTypeToSapper");
	}

	THINK_FUNC_DECL(SetBuildableObjectTypes)
	{
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);
		auto kv = builder->GetItem()->GetStaticData()->GetKeyValues()->FindKey("used_by_classes");
		int typesAllowed = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(builder, typesAllowed, allowed_build_types);
		if (typesAllowed == 0) {
			bool isSapper = kv != nullptr && kv->FindKey("spy") != nullptr;
			typesAllowed = isSapper ? 8: 7;
		}
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 1) == 1, 0);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 2) == 2, 1);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 4) == 4, 2);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 8) == 8, 3);
		if (typesAllowed & 8) {
			builder->SetSubType(3);
			builder->m_iObjectMode = 0;
			// Keep resetting the type to sapper mode
			if (typesAllowed != 8) {
				THINK_FUNC_SET(reinterpret_cast<CTFWeaponBuilder *>(this), SetTypeToSapper, gpGlobals->curtime + 0.5f);
			}
		}
	}
	VHOOK_DECL(void, CTFWeaponBuilder_Equip, CBaseCombatCharacter *owner)
	{
		VHOOK_CALL(CTFWeaponBuilder_Equip)(owner);
		THINK_FUNC_SET(reinterpret_cast<CTFWeaponBuilder *>(this), SetBuildableObjectTypes, gpGlobals->curtime);
	}
	VHOOK_DECL(void, CTFWeaponSapper_Equip, CBaseCombatCharacter *owner)
	{
		VHOOK_CALL(CTFWeaponSapper_Equip)(owner);
		THINK_FUNC_SET(reinterpret_cast<CTFWeaponSapper *>(this), SetBuildableObjectTypes, gpGlobals->curtime);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (FStrEq(args[0], "destroy") && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER) {
			
			int iBuilding = 0;
			int iMode = 0;
			bool bArgsChecked = false;

			// Fixup old binds.
			if ( args.ArgC() == 2 )
			{
				iBuilding = atoi( args[ 1 ] );
				if ( iBuilding == 3 ) // Teleport exit is now a mode.
				{
					iBuilding = 1;
					iMode = 1;
				}
				bArgsChecked = true;
			}
			else if ( args.ArgC() == 3 )
			{
				iBuilding = atoi( args[ 1 ] );
				if (iBuilding == 3) return false; // No destroying sappers
				iMode = atoi( args[ 2 ] );
				bArgsChecked = true;
			}

			if ( bArgsChecked )
			{
				player->DetonateObjectOfType( iBuilding, iMode );
			}
			else
			{
				ClientMsg(player, "Usage: destroy <building> <mode>\n" );
			}
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}

	DETOUR_DECL_MEMBER(int, CTFItemDefinition_GetLoadoutSlot, int classIndex)
	{
		auto item_def = reinterpret_cast<CTFItemDefinition *>(this);
		if (classIndex == TF_CLASS_UNDEFINED && StringStartsWith(item_def->GetItemClass(), "tf_weapon_revolver")) return LOADOUT_POSITION_PRIMARY;

		return DETOUR_MEMBER_CALL(CTFItemDefinition_GetLoadoutSlot)(classIndex);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Unintended_Class_Weapon_Improvements")
		{
			// Allow to use viewmodels of the original class
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_Equip,     "CBaseCombatWeapon::Equip");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Deploy,     "CTFWeaponBase::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Holster,     "CTFWeaponBase::Holster");
			MOD_ADD_DETOUR_MEMBER(CEconEntity_UpdateOnRemove,     "CEconEntity::UpdateOnRemove");

			// Allow non demos to use shields and benefit from eyelander heads
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoClassSpecialSkill, "CTFPlayer::DoClassSpecialSkill");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_UpdateChargeMeter, "CTFPlayerShared::UpdateChargeMeter");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_EndClassSpecialSkill, "CTFPlayer::EndClassSpecialSkill");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TeamFortress_CalculateMaxSpeed, "CTFPlayer::TeamFortress_CalculateMaxSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxHealthForBuffing, "CTFPlayer::GetMaxHealthForBuffing");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_SecondaryAttack, "CTFMinigun::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFFists_SecondaryAttack, "CTFFists::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFFists_Punch, "CTFFists::Punch");

			// Allow other classes to build and destroy buildings
			MOD_ADD_DETOUR_STATIC(ClassCanBuild, "ClassCanBuild");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBuilder_SetSubType, "CTFWeaponBuilder::SetSubType");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ManageBuilderWeapons, "CTFPlayer::ManageBuilderWeapons");
			MOD_ADD_VHOOK(CTFWeaponBuilder_Equip, TypeName<CTFWeaponBuilder>(), "CTFWeaponBase::Equip");
			MOD_ADD_VHOOK(CTFWeaponSapper_Equip, TypeName<CTFWeaponSapper>(), "CTFWeaponBase::Equip");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");

			// Allow to taunt with some other class weapons
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Taunt,     "CTFPlayer::Taunt");

			// Let revolver weapon to be used with other secondary weapon on non spy
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFItemDefinition_GetLoadoutSlot,     "CTFItemDefinition::GetLoadoutSlot", LOWEST);
        }
    } s_Mod;
    
    ConVar cvar_enable("sig_etc_unintended_class_weapon_improvements", "0", FCVAR_NOTIFY,
		"Mod: allow all classes to fully benefit from shield, eyelander, tf_weapon_builder",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
		
}