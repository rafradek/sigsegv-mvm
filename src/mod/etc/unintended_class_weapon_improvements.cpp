#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfentities.h"
#include "stub/tfplayer.h"
#include "stub/extraentitydata.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/iterate.h"
#include "stub/misc.h"
#include <fmt/format.h>
#include "mod/item/item_common.h"
#include "stub/gamerules.h"
#include "mod/common/text_hud.h"


namespace Mod::Etc::Unintended_Class_Weapon_Improvements
{

	
	class UnintendedClassViewmodelOverride : public EntityModule
	{
	public:
		UnintendedClassViewmodelOverride(CBaseEntity *entity) : EntityModule(entity) {};

		CHandle<CBaseEntity> wearable;
		CHandle<CBaseEntity> wearableWeapon;
		int oldModelIndex;
		const char *properHandModel = nullptr;
		int properHandModelIndex;
		CHandle<CBaseEntity> playerModelWearable;
		const char *properPlayerModel = nullptr;
		const char *oldPlayerModel = nullptr;
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
		
		if (mod->wearable != nullptr) {
			mod->wearable->Remove();
		}
		if (mod->wearableWeapon != nullptr) {
			mod->wearableWeapon->Remove();
		}
		if (mod->playerModelWearable != nullptr) {
			mod->playerModelWearable->Remove();
		}

		if (mod->properPlayerModel != nullptr) {
			auto wearable_player = static_cast<CTFWearable *>(CreateEntityByName("tf_wearable"));
			wearable_player->Spawn();
			wearable_player->GiveTo(owner);
			wearable_player->SetModelIndex(CBaseEntity::PrecacheModel(mod->oldPlayerModel));
			wearable_player->m_bValidatedAttachedEntity = true;
			mod->playerModelWearable = wearable_player;
			owner->GetPlayerClass()->SetCustomModel(mod->properPlayerModel);
			owner->m_nRenderFX = 6;
		}

		if (weapon->m_nCustomViewmodelModelIndex != mod->properHandModelIndex) return false;
		if (mod->properHandModel == nullptr) return true;

		weapon->SetModel(mod->properHandModel);
		weapon->m_iViewModelIndex = mod->properHandModelIndex;
		auto wearable_vm = static_cast<CTFWearable *>(CreateEntityByName("tf_wearable_vm"));
		wearable_vm->Spawn();
		wearable_vm->GiveTo(owner);
		wearable_vm->SetModelIndex(mod->oldModelIndex);
		wearable_vm->m_bValidatedAttachedEntity = true;
		mod->wearable = wearable_vm;

        // Don't create wearable imitating weapon model if a custom model is being used
		const char *customModel = weapon->GetAttributeManager()->ApplyAttributeStringWrapper(NULL_STRING, weapon, PStrT<"custom_item_model">()).ToCStr();
        if (customModel == nullptr || customModel[0] == '\0' ) {
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
		if (mod->playerModelWearable != nullptr) {
			mod->playerModelWearable->Remove();
		}
		if (mod->properPlayerModel != nullptr) {
			owner->GetPlayerClass()->SetCustomModel(mod->oldPlayerModel);
			owner->m_nRenderFX = 0;
		}
		return true;
	}

    ConVar cvar_enable_viewmodel("sig_etc_unintended_class_weapon_viewmodel", "0", FCVAR_NOTIFY,
		"Mod: use proper class viewmodel animations for unintended player class weapons");

    ConVar cvar_enable_playermodel("sig_etc_unintended_class_weapon_player_model", "0", FCVAR_NOTIFY,
		"Mod: use proper class player animations for unintended player class weapons");

    ConVar sig_etc_unintended_class_weapon_fix_ammo("sig_etc_unintended_class_weapon_fix_ammo", "1", FCVAR_NOTIFY,
		"Mod: fix max ammo count for unintented player class weapons");

    DETOUR_DECL_MEMBER(void, CTFWeaponBase_UpdateHands)
	{
		DETOUR_MEMBER_CALL(CTFWeaponBase_UpdateHands)();
		
		auto ent = reinterpret_cast<CBaseCombatWeapon *>(this);
		auto weapon = rtti_cast<CTFWeaponBase *>(ent);

		if (weapon == nullptr || weapon->GetItem() == nullptr) return;

		auto owner = weapon->GetTFPlayerOwner();
		auto def = weapon->GetItem()->GetItemDefinition();

		if (owner == nullptr) return;

		auto classIndex = owner->GetPlayerClass()->GetClassIndex();

		int otherClassViewmodel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, otherClassViewmodel, use_original_class_weapon_animations);

		int otherClassPlayerModel = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, otherClassPlayerModel, use_original_class_player_animations);

		if (cvar_enable_viewmodel.GetBool() || otherClassViewmodel != 0 || otherClassPlayerModel != 0) {
			
			// Use viewmodel of a real class if applicable
			if (((owner->IsRealPlayer() && owner->GetViewModel() != nullptr) || (otherClassPlayerModel != 0 || cvar_enable_playermodel.GetBool())) && weapon->m_nViewModelIndex == 0) {
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
						if ((otherClassViewmodel != 0 || cvar_enable_viewmodel.GetBool()) && owner->GetViewModel() != nullptr) {
							mod->oldModelIndex = weapon->GetModelIndex();
							int oldClass = owner->GetPlayerClass()->GetClassIndex();
							owner->GetPlayerClass()->SetClassIndex(properClass);
							mod->properHandModel = weapon->GetViewModel(); //GetPlayerClassData(properClass)->m_szHandModelName;
							owner->GetPlayerClass()->SetClassIndex(oldClass);
							mod->properHandModelIndex = CBaseEntity::PrecacheModel(mod->properHandModel);
							weapon->SetCustomViewModel(mod->properHandModel);
							weapon->m_iViewModelIndex = mod->properHandModelIndex;
							weapon->SetModel(mod->properHandModel);
						}
						if (otherClassPlayerModel != 0 || cvar_enable_playermodel.GetBool()) {
							mod->properPlayerModel = GetPlayerClassData(properClass)->m_szModelName;
							mod->oldPlayerModel = owner->GetPlayerClass()->GetCustomModel();
						}
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

		// Fix up ammo count
		if (sig_etc_unintended_class_weapon_fix_ammo.GetBool()) {
			int ammoType = weapon->m_iPrimaryAmmoType;
#ifndef NO_MVM
			Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(true);
#endif
			if ((ammoType == TF_AMMO_PRIMARY || ammoType == TF_AMMO_SECONDARY) && def->GetLoadoutSlot(classIndex) == -1) {
				int properClass = -1;
				for (int i = 1; i < 10; i++) {
					if (def->GetLoadoutSlot(i) != -1) {
						properClass = i;
						break;
					}
				}
				if (properClass != -1) {

					int ammoProper = GetPlayerClassData(properClass)->m_aAmmoMax[ammoType];
					int ammoOur = GetPlayerClassData(classIndex)->m_aAmmoMax[ammoType];
					const char *attrib;
					float value;
					if (ammoOur != 0) {
						attrib = ammoType == TF_AMMO_PRIMARY ? "max ammo primary mult" : "max ammo secondary mult";
						value = (float)ammoProper / (float)ammoOur;
					}
					else {
						attrib = ammoType == TF_AMMO_PRIMARY ? "max ammo primary additive" : "max ammo secondary additive";
						value = (float)ammoProper;
					}
					auto attrDef = GetItemSchema()->GetAttributeDefinitionByName(attrib);
					weapon->GetItem()->GetAttributeList().SetRuntimeAttributeValue(attrDef, value);
				}
			}
#ifndef NO_MVM
			Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(false);
#endif
		}

#ifndef NO_MVM
		Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(true);
#endif
		if (def->GetLoadoutSlot(classIndex) == -1 && !weapon->m_bDisguiseWeapon) {
			weapon->SetCustomVariable("isunintendedclass", Variant(true));
		}
#ifndef NO_MVM
		Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(false);
#endif
	} 

	DETOUR_DECL_MEMBER(void, CTFWearable_Equip, CBasePlayer *player)
	{
		CTFWearable *wearable = reinterpret_cast<CTFWearable *>(this);

#ifndef NO_MVM
		Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(true);
#endif
		if (wearable->GetItem() != nullptr && !wearable->m_bDisguiseWearable && wearable->GetItem()->GetItemDefinition()->GetLoadoutSlot(ToTFPlayer(player)->GetPlayerClass()->GetClassIndex()) == -1) {
			wearable->SetCustomVariable("isunintendedclass", Variant(true));
		}
#ifndef NO_MVM
		Mod::Pop::PopMgr_Extensions::DisableLoadoutSlotReplace(false);
#endif
		DETOUR_MEMBER_CALL(CTFWearable_Equip)(player);
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
        if (!player->m_Shared->InCond(TF_COND_TAUNTING) && player->GetActiveTFWeapon() != nullptr && player->GetActiveTFWeapon()->GetItem() != nullptr && player->GetActiveTFWeapon()->GetItem()->GetItemDefinition()->GetLoadoutSlot(playerClass) == -1) {
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
		
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_SCOUT && player->Weapon_OwnsThisID(TF_WEAPON_PEP_BRAWLER_BLASTER)) {
			ret *= RemapValClamped( player->m_Shared->m_flHypeMeter, 0.0f, 100.0f, 1.0f, 1.45f );
		}

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

	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale)
	{
		if (pPlayer == nullptr) {
			DETOUR_STATIC_CALL(HandleRageGain)(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale); 
			return;
		}

		int restoreClass = -1;
		int classIndex = pPlayer->GetPlayerClass()->GetClassIndex();
		if ((0x08 /*kRageBuffFlag_OnBurnDamageDealt*/ & iRequiredBuffFlags) && classIndex != TF_CLASS_PYRO) {
			auto weapon = rtti_cast<CTFFlameThrower *>(pPlayer->GetEntityForLoadoutSlot(LOADOUT_POSITION_PRIMARY));
			if (weapon != nullptr && CAttributeManager::AttribHookValue<int>(0, "set_buff_type", weapon) != 0) {
				pPlayer->GetPlayerClass()->SetClassIndex(TF_CLASS_PYRO);
				restoreClass = classIndex;
			}
		}
		if ((0x01 /*kRageBuffFlag_OnDamageDealt*/ & iRequiredBuffFlags) && classIndex != TF_CLASS_SOLDIER) {
			auto weapon = rtti_cast<CTFBuffItem *>(pPlayer->GetEntityForLoadoutSlot(LOADOUT_POSITION_SECONDARY));
			if (weapon != nullptr && CAttributeManager::AttribHookValue<int>(0, "set_buff_type", weapon) != 0) {
				pPlayer->GetPlayerClass()->SetClassIndex(TF_CLASS_SOLDIER);
				restoreClass = classIndex;
			}
		}
		if ((0x10 /*kRageBuffFlag_OnHeal*/ & iRequiredBuffFlags) && classIndex != TF_CLASS_MEDIC) {
			auto weapon = rtti_cast<CWeaponMedigun *>(pPlayer->GetEntityForLoadoutSlot(LOADOUT_POSITION_SECONDARY));
			if (weapon != nullptr && CAttributeManager::AttribHookValue<int>(0, "generate_rage_on_heal", weapon) != 0) {
				pPlayer->GetPlayerClass()->SetClassIndex(TF_CLASS_MEDIC);
				restoreClass = classIndex;
			}
		}
		if ((0x01 /*kRageBuffFlag_OnDamageDealt*/ & iRequiredBuffFlags) && classIndex != TF_CLASS_HEAVYWEAPONS) {
			auto weapon = rtti_cast<CTFMinigun *>(pPlayer->GetActiveTFWeapon());
			if (weapon != nullptr && CAttributeManager::AttribHookValue<int>(0, "generate_rage_on_dmg", weapon) != 0) {
				pPlayer->GetPlayerClass()->SetClassIndex(TF_CLASS_HEAVYWEAPONS);
				restoreClass = classIndex;
			}
		}
		DETOUR_STATIC_CALL(HandleRageGain)(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale);
		if (restoreClass != -1) {
			pPlayer->GetPlayerClass()->SetClassIndex(restoreClass);
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_UpdateEnergyDrinkMeter)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
		int restoreClass = -1;
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_SCOUT && (rtti_cast<CTFLunchBox_Drink *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_SECONDARY)) != nullptr 
			|| rtti_cast<CTFSodaPopper *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_PRIMARY)) != nullptr)) {
			restoreClass = player->GetPlayerClass()->GetClassIndex();
			player->GetPlayerClass()->SetClassIndex(TF_CLASS_SCOUT);
		}
		DETOUR_MEMBER_CALL(CTFPlayerShared_UpdateEnergyDrinkMeter)();
		if (restoreClass != -1) {
			player->GetPlayerClass()->SetClassIndex(restoreClass);
		}
	}

	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr && strcmp(event->GetName(), "player_death") == 0) {
			auto assister = ToTFPlayer(UTIL_PlayerByUserId(event->GetInt("assister")));
			auto attacker = ToTFPlayer(UTIL_PlayerByUserId(event->GetInt("attacker")));
			if (assister != nullptr && assister != attacker && assister->GetPlayerClass()->GetClassIndex() != TF_CLASS_SNIPER) {
				auto weapon = assister->GetActiveTFWeapon();
				float rageOnAssist = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, rageOnAssist, rage_on_assists);
				if (rageOnAssist != 0) {
					assister->m_Shared->m_flRageMeter = Min(100.0f, assister->m_Shared->m_flRageMeter + rageOnAssist);
				}
			}
			if (attacker != nullptr && attacker->GetPlayerClass()->GetClassIndex() != TF_CLASS_SNIPER) {
				auto weapon = attacker->GetActiveTFWeapon();
				float rageOnKill = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, rageOnKill, rage_on_kill);
				if (rageOnKill != 0) {
					attacker->m_Shared->m_flRageMeter = Min(100.0f, attacker->m_Shared->m_flRageMeter + rageOnKill);
				}
			}
		}
		return DETOUR_MEMBER_CALL(IGameEventManager2_FireEvent)(event, bDontBroadcast);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanAirDash)
	{
		bool ret = DETOUR_MEMBER_CALL(CTFPlayer_CanAirDash)();
		if (!ret) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (!player->IsPlayerClass(TF_CLASS_SCOUT) && player->m_Shared->InCond(TF_COND_SODAPOPPER_HYPE)) {
				return player->m_Shared->m_iAirDash < 5;
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, CTakeDamageInfo &info)
	{
		int damage = DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage)(info);
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_SCOUT)
		{
			// Lose hype on take damage
			int iHypeResetsOnTakeDamage = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, iHypeResetsOnTakeDamage, lose_hype_on_take_damage);
			if ( iHypeResetsOnTakeDamage != 0 )
			{
				// Loose x hype on jump
				player->m_Shared->m_flHypeMeter = Max(0.0f, player->m_Shared->m_flHypeMeter - iHypeResetsOnTakeDamage * info.GetDamage());
				player->TeamFortress_SetSpeed();
			}
		}
		return damage;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxAmmo, int ammoIndex, int classIndex)
	{
		auto ret = DETOUR_MEMBER_CALL(CTFPlayer_GetMaxAmmo)(ammoIndex, classIndex);
		if (ammoIndex == TF_AMMO_GRENADES1 && ret == 0) {
			return 1;
		}
		if (ammoIndex == TF_AMMO_GRENADES2 && ret == 0) {
			return 1;
		}
		return ret;
	}
	
    ConVar sig_etc_unintended_class_weapon_display_meters("sig_etc_unintended_class_weapon_display_meters", "1", FCVAR_NOTIFY,
		"Mod: display meters for unintended player class weapons");

    class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:Unintended_Class_Weapon_Improvements")
		{
			// Allow to use viewmodels of the original class
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFWeaponBase_UpdateHands,     "CTFWeaponBase::UpdateHands", HIGH);
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Deploy,     "CTFWeaponBase::Deploy");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_Holster,     "CTFWeaponBase::Holster");
			MOD_ADD_DETOUR_MEMBER(CEconEntity_UpdateOnRemove,     "CEconEntity::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CTFWearable_Equip,     "CTFWearable::Equip");

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

			// Allow HandleRageGain to work with weapons on other classes
			MOD_ADD_DETOUR_STATIC(HandleRageGain,     "HandleRageGain");

			// Let revolver weapon to be used with other secondary weapon on non spy
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFItemDefinition_GetLoadoutSlot,     "CTFItemDefinition::GetLoadoutSlot", LOWEST);

			// Fix drink and soda popper usage on non scout
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_UpdateEnergyDrinkMeter, "CTFPlayerShared::UpdateEnergyDrinkMeter");

			// Make focus work on other classes
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");

			// Make soda popper hype work on non scout
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanAirDash, "CTFPlayer::CanAirDash");

			// Lose hype on take damage for all classes
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");

			// Allow grenades from other items
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxAmmo, "CTFPlayer::GetMaxAmmo");
			
        }

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled() && sig_etc_unintended_class_weapon_display_meters.GetBool(); }

        virtual void FrameUpdatePostEntityThink() override
        {
			// Draw meters
			for (int i = gpGlobals->tickcount % 4; i < gpGlobals->maxClients; i+=4) {
				auto player = ToTFPlayer(UTIL_PlayerByIndex(i));
				if (player == nullptr || !player->IsRealPlayer()) continue;

				std::string message;
				auto classIndex = player->GetPlayerClass()->GetClassIndex();
				ForEachTFPlayerEconEntity(player, [player, classIndex, &message](CEconEntity *item){
					if (!item->GetCustomVariableBool<"isunintendedclass">()) return;

					auto weapon = rtti_cast<CTFWeaponBase *>(item);
					if (weapon == nullptr) {
						if (classIndex != TF_CLASS_DEMOMAN && rtti_cast<CTFWearableDemoShield *>(item) != nullptr) {
							message += fmt::format("Charge: {:.0f}%\n", player->m_Shared->m_flChargeMeter.Get());
						}
					}
					auto weaponid = 0;
					if (weapon != nullptr) {
						const char *itemClass = item->GetClassname();
						weaponid = weapon->GetWeaponID();
						if (classIndex != TF_CLASS_DEMOMAN && weaponid == TF_WEAPON_PIPEBOMBLAUNCHER) {
							message += fmt::format("Stickies: {}\n", rtti_cast<CTFPipebombLauncher *>(item)->m_iPipebombCount.Get());
						}
						else if (classIndex != TF_CLASS_MEDIC && weaponid == TF_WEAPON_MEDIGUN) {
							message += fmt::format("Ubercharge: {}%\n", (int)(rtti_cast<CWeaponMedigun *>(item)->GetCharge() * 100));
							int generate = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(item, generate, generate_rage_on_heal);
							if (generate != 0) {
								message += fmt::format("Shield: {:.0f}%\n", player->m_Shared->m_flRageMeter.Get());
							}
						}
						else if (classIndex != TF_CLASS_PYRO && weaponid == TF_WEAPON_FLAMETHROWER) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(item, buff, set_buff_type);
							if (buff != 0) {
								message += fmt::format("MMMPH: {:.0f}%\n", player->m_Shared->m_flRageMeter.Get());
							}
						}
						else if (classIndex != TF_CLASS_SOLDIER && weaponid == TF_WEAPON_BUFF_ITEM) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(item, buff, set_buff_type);
							if (buff != 4 && buff != 0) {
								message += fmt::format("Rage: {:.0f}%\n", player->m_Shared->m_flRageMeter.Get());
							}
						}
						else if (classIndex != TF_CLASS_HEAVYWEAPONS && weaponid == TF_WEAPON_MINIGUN) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(item, buff, generate_rage_on_dmg);
							if (buff != 0) {
								message += fmt::format("Rage: {:.0f}%\n", player->m_Shared->m_flRageMeter.Get());
							}
						}
						// else if (classIndex != TF_CLASS_SCOUT && weaponid == TF_WEAPON_LUNCHBOX) {
						// 	message += fmt::format("Drink: {:.0f}%\n", player->m_Shared->m_flEnergyDrinkMeter.Get());
						// }
						// else if (classIndex != TF_CLASS_SCOUT && rtti_cast<CTFLunchBox_Drink *>(item) != nullptr) {
						// 	message += fmt::format("Drink: {:.0f}%\n", player->m_Shared->m_flEnergyDrinkMeter.Get());
						// }
						else if (classIndex != TF_CLASS_SCOUT && (weaponid == TF_WEAPON_SODA_POPPER || weaponid == TF_WEAPON_PEP_BRAWLER_BLASTER)) {
							message += fmt::format("Hype: {:.0f}%\n", round(player->m_Shared->m_flHypeMeter.Get()));
						}
						else if (classIndex != TF_CLASS_SOLDIER && weaponid == TF_WEAPON_ROCKETLAUNCHER && rtti_cast<CTFRocketLauncher_AirStrike *>(weapon) != nullptr) {
							message += fmt::format("Kills: {}\n", player->m_Shared->m_iDecapitations.Get());
						}
						else if (classIndex != TF_CLASS_DEMOMAN && weaponid == TF_WEAPON_SWORD && rtti_cast<CTFSword *>(weapon) != nullptr) {
							message += fmt::format("Heads: {}\n", player->m_Shared->m_iDecapitations.Get());
						}
						else if (classIndex != TF_CLASS_SNIPER && weaponid == TF_WEAPON_SNIPERRIFLE_DECAP) {
							message += fmt::format("Heads: {}\n", player->m_Shared->m_iDecapitations.Get());
						}
						else if (classIndex != TF_CLASS_SNIPER && weaponid == TF_WEAPON_SNIPERRIFLE) {
							float buff = 0;
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, buff, set_buff_type);
							if (buff != 0) {
								message += fmt::format("Focus: {:.0f}%\n", player->m_Shared->m_flRageMeter.Get());
							}
						}
						else if (classIndex != TF_CLASS_SNIPER && weaponid == TF_WEAPON_CHARGED_SMG) {
							float buff = 0;
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, buff, minicrit_boost_charge_rate);
							if (buff != 0) {
								message += fmt::format("Crikey: {:.0f}%\n", rtti_cast<CTFChargedSMG *>(item)->m_flMinicritCharge.Get());
							}
						}
						else if (classIndex != TF_CLASS_ENGINEER && weaponid == TF_WEAPON_SENTRY_REVENGE) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, buff, sentry_killed_revenge);
							if (buff != 0) {
								message += fmt::format("Crits: {}\n", player->m_Shared->m_iRevengeCrits.Get());
							}
						}
						else if ((classIndex != TF_CLASS_PYRO && weaponid == TF_WEAPON_FLAREGUN_REVENGE)) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, buff, extinguish_revenge);
							if (buff != 0) {
								message += fmt::format("Crits: {}\n", player->m_Shared->m_iRevengeCrits.Get());
							}
						}
						else if (classIndex != TF_CLASS_SPY && weaponid == TF_WEAPON_REVOLVER) {
							int buff = 0;
							CALL_ATTRIB_HOOK_INT_ON_OTHER(item, buff, sapper_kills_collect_crits);
							if (buff != 0) {
								message += fmt::format("Crits: {}\n", player->m_Shared->m_iRevengeCrits.Get());
							}
							
						}
						float regen = weapon->InternalGetEffectBarRechargeTime();
						if (regen != 0.0f) {
							message += fmt::format("{}: {:.0f}%\n", GetItemNameForDisplay(item->GetItem()), round(weapon->GetEffectBarProgress() * 100));
						}
					}
					auto meterItem = rtti_cast<IHasGenericMeter *>(item);
					if (meterItem != nullptr && weaponid != TF_WEPON_FLAME_BALL) {
						int type = 0;
						CALL_ATTRIB_HOOK_INT_ON_OTHER(item, type, item_meter_charge_type);
						if (type != 0) {
							message += fmt::format("{}: {:.0f}%\n", GetItemNameForDisplay(item->GetItem()), round(player->m_Shared->m_flItemChargeMeter[item->GetItem()->GetItemDefinition()->GetLoadoutSlot(TF_CLASS_UNDEFINED)]));
						}
					}
				});

				auto activeWeapon = player->GetActiveTFWeapon();
				if (activeWeapon != nullptr) {
					if (activeWeapon->IsEnergyWeapon() && activeWeapon->GetItem()->GetItemDefinition()->GetLoadoutSlot(classIndex) == -1) {
						message += fmt::format("Energy: {:.0f}%\n", round(activeWeapon->m_flEnergy / activeWeapon->Energy_GetMaxEnergy() * 100));
					}
				}
				
				if (message.empty()) continue;

				hudtextparms_t textparam;
				textparam.channel = 1;
				textparam.x = 0.8f;
				textparam.y = 0.75f;
				textparam.effect = 0;
				textparam.r1 = 255;
				textparam.r2 = 255;
				textparam.b1 = 255;
				textparam.b2 = 255;
				textparam.g1 = 255;
				textparam.g2 = 255;
				textparam.a1 = 0;
				textparam.a2 = 0; 
				textparam.fadeinTime = 0.f;
				textparam.fadeoutTime = 0.15f;
				textparam.holdTime = 0.15f;
				textparam.fxTime = 1.0f;
				DisplayHudMessageAutoChannel(player, textparam, message.c_str(), 1);
			}
        }
    } s_Mod;
    
    ConVar cvar_enable("sig_etc_unintended_class_weapon_improvements", "0", FCVAR_NOTIFY,
		"Mod: allow all classes to fully benefit from shield, eyelander, tf_weapon_builder, and others. Must be set for the above 4 convars to work",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
		
}