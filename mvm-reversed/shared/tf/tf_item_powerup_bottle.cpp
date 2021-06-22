/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/tf/tf_item_powerup_bottle.cpp
 */


void CTFPowerupBottle::ReapplyProvision()
{
	CTFWearable::ReapplyProvision();
	
	CBaseEntity *owner = this->GetOwnerEntity();
	if (owner == nullptr) {
		return;
	}
	
	IHasAttributes *owner_ihasattr = owner->GetHasAttributesInterfacePtr();
	if (owner_ihasattr == nullptr) {
		return;
	}
	
	if (this->m_bActive) {
		if (!owner_ihasattr->GetAttributeManager()->IsBeingProvidedToBy(this)) {
			this->GetAttributeManager()->ProvideTo(owner);
		}
	} else {
		this->GetAttributeManager()->StopProvidingTo(owner);
	}
	
	CTFPlayer *player = dynamic_cast<CTFPlayer *>(owner);
	if (player == nullptr) {
		return;
	}
	
	/* BUG: because "canteen_specialist" is an integer attribute, the call to
	 * AttribHookValue<int> will truncate any fractional part of the powerup
	 * duration */
	float duration = CAttributeManager::AttribHookValue<float>(0.0f,
		"powerup_duration", this, nullptr, true);
	duration = CAttributeManager::AttribHookValue<int>((int)duration,
		"canteen_specialist", player, nullptr, true);
	
	CTFPlayer *share_with = nullptr;
	int share_attr = 0;
	
	if (player->IsPlayerClass(TF_CLASS_MEDIC)) {
		CTFWeaponBase *weapon = player->GetActiveWeapon();
		if (weapon != nullptr) {
			CWeaponMedigun *medigun = dynamic_cast<CWeaponMedigun *>(weapon);
			if (medigun != nullptr) {
				share_with = ToTFPlayer(medigun->GetHealTarget());
				if (share_with != nullptr) {
					share_attr = CAttributeManager::AttribHookValue<int>(0,
						"canteen_specialist", player, nullptr, true);
				}
			}
		}
	}
	
	bool did_share = false;
	
	if (CAttributeManager::AttribHookValue<int>(0, "critboost",
		this, nullptr, true) != 0) {
		if (this->m_bActive) {
			player->m_Shared.AddCond(TF_COND_CRITBOOSTED_USER_BUFF, duration);
			
			if (share_with != nullptr && share_attr != 0) {
				share_with->m_Shared.AddCond(TF_COND_CRITBOOSTED_USER_BUFF,
					duration);
				did_share = true;
			}
		} else {
			player->m_Shared.RemoveCond(TF_COND_CRITBOOSTED_USER_BUFF, true);
		}
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "ubercharge",
		this, nullptr, true) != 0) {
		if (this->m_bActive) {
			player->m_Shared.AddCond(TF_COND_INVULNERABLE_USER_BUFF, duration);
			
			if (player->IsPlayerClass(TF_CLASS_ENGINEER)) {
				int obj_count = player->GetObjectCount();
				if (obj_count > 0) {
					for (int i = obj_count - 1; i != -1; --i) {
						CBaseObject *obj = player->GetObject(i);
						if (obj != nullptr) {
							CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>(obj);
							if (sentry != nullptr && !sentry->m_bCarried) {
								sentry->m_nShieldLevel = 2;
								// TODO: set float @ CObjectSentrygun+0xb14
								// to gpGlobals->curtime + duration
							}
						}
					}
				}
			} else if (share_with != nullptr && share_attr != 0) {
				share_with->m_Shared.AddCond(TF_COND_INVULNERABLE_USER_BUFF,
					duration);
				did_share = true;
			}
		} else {
			player->m_Shared.RemoveCond(TF_COND_INVULNERABLE_USER_BUFF, true);
		}
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "recall",
		this, nullptr, true) != 0) {
		if (this->m_bActive) {
			player->ForceRespawn();
			player->m_Shared.AddCond(TF_COND_SPEED_BOOST, 7.0f);
		}
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "refill_ammo",
		this, nullptr, true) != 0) {
		if (this->m_bActive) {
			for (int i = 0; i < MAX_WEAPONS; ++i) {
				CBaseCombatWeapon *weapon = player->GetWeapon(i);
				if (weapon == nullptr) {
					continue;
				}
				
				/* BUG: as soon as this loop hits a weapon which is completely
				 * devoid of ammo (clip 0, reserve 0), it will
				 * (a) fail to refill the clip for that weapon, and
				 * (b) fail to refill the clip for any weapons in later slots
				 * NOTE: for the purposes of this, energy weapons are treated as
				 * if their reserve ammo is always empty
				 */
				if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() &&
					((weapon->UsesPrimaryAmmo() && !weapon->HasPrimaryAmmo()) ||
					(weapon->UsesSecondaryAmmo() && !weapon->HasSecondaryAmmo()))) {
					player->AwardAchievement(TF_MVM_USE_AMMO_BOTTLE);
					break;
				}
				
				/* BUG: doesn't refill clip of energy weapons
				 * (should vcall weapon->IsEnergyWeapon,
				 * then call weapon->Energy_Recharge
				 * until weapon->Energy_FullyCharged) */
				
				
				// CBaseCombatWeapon::UsesPrimaryAmmo:
				// - if m_iPrimaryAmmoType < 0: return false
				// - else:                      return true
				// CBaseCombatWeapon::UsesSecondaryAmmo:
				// - if m_iSecondaryAmmoType < 0: return false
				// - else:                        return true
				
				// CTFWeaponBaseGun::HasPrimaryAmmo:
				// - there's a special case if m_iPrimaryAmmoType == TF_AMMO_METAL (e.g. Widowmaker)
				// - if UsesClipsForAmmo1(): return (m_iClip1 > 0) || (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
				// - else:                   return (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
				// CBaseCombatWeapon::HasSecondaryAmmo:
				// - if UsesClipsForAmmo1(): return (m_iClip2 > 0) || (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0)
				// - else:                   return (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0)
				
			
				weapon->GiveDefaultAmmo();
				
				if (share_with != nullptr && share_attr != 0) {
					CBaseCombatWeapon *share_weapon = share_with->GetWeapon(i);
					if (share_weapon != nullptr) {
						share_weapon->GiveDefaultAmmo();
						did_share = true;
					}
				}
			}
			
			/* NOTE: invented TF_AMMO_SOURCE_RESUPPLY for (EAmmoSource)1 */
			if (share_with != nullptr && share_attr != 0) {
				for (int i = 0; i < TF_AMMO_COUNT; ++i) {
					player->GiveAmmo(player->GetMaxAmmo(i), i, true,
						TF_AMMO_SOURCE_RESUPPLY);
					share_with->GiveAmmo(share_with->GetMaxAmmo(i), i, true,
						TF_AMMO_SOURCE_RESUPPLY);
				}
				did_share = true;
			} else {
				for (int i = 0; i < TF_AMMO_COUNT; ++i) {
					player->GiveAmmo(player->GetMaxAmmo(i), i, true,
						TF_AMMO_SOURCE_RESUPPLY);
				}
			}
		}
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "building_instant_upgrade",
		this, nullptr, true) != 0) {
		if (this->m_bActive) {
			int obj_count = player->GetObjectCount();
			if (obj_count > 0) {
				for (int i = obj_count - 1; i != -1; --i) {
					CBaseObject *obj = player->GetObject(i);
					if (obj != nullptr) {
						int max_level = obj->GetMaxUpgradeLevel();
						
						if (obj->m_bCarried) {
							/* BUG: this can't possibly be right */
							obj->m_iHighestUpgradeLevel = obj->GetMaxUpgradeLevel();
						} else if (obj->GetUpgradeLevel() == max_level) {
							obj->SetHealth(obj->GetMaxHealth());
						} else {
							if (TFGameRules() && TFGameRules()->IsMannVsMachineMode()) {
								// TODO: event mvm_quick_sentry_upgrade
							}
							obj->DoQuickBuild(true);
						}
					}
				}
			}
		}
	}
	
	if (did_share) {
		// TODO: event mvm_medic_powerup_shared
	}
}


bool CTFPowerupBottle::Use()
{
	/* yes, they really did just declare a class inside a function definition */
	class CAttributeIterator_ConsumeOneRefundableCharge :
		public IEconItemUntypedAttributeIterator
	{
	public:
		CAttributeIterator_ConsumeOneRefundableCharge(
			const CAttributeList *attrlist, int num_charges)
			: m_pAttrList(attrlist), m_iNumCharges(num_charges) { }
		virtual ~CAttributeIterator_ConsumeOneRefundableCharge() { }
		
		virtual bool OnIterateAttributeValueUntyped(const CEconItemAttributeDefinition *attr)
		{
			if (FindAttribute<CAttributeList>(this->m_pAttrList, attr)) {
				int currency = this->m_pAttrList->
					GetRuntimeAttributeRefundableCurrency(attr);
				if (currency > 0) {
					this->m_pAttrList-> SetRuntimeAttributeRefundableCurrency(
						attr, currency - (currency / this->m_iNumCharges));
				}
			}
			
			return true;
		}
		
	private:
		const CAttributeList *m_pAttrList; // +0x04
		int m_iNumCharges;                 // +0x08
	};
	
	
	if (this->m_bActive || this->m_usNumCharges == 0 || !this->AllowedToUse()) {
		return false;
	}
	
	CAttributeIterator_ConsumeOneRefundableCharge it(this->GetAttributeList(),
		this->m_usNumCharges);
	this->GetAttributeList()->IterateAttributes(it);
	
	float duration = CAttributeManager::AttribHookValue<float>(0.0f,
		"powerup_duration", this, nullptr, true);
	
	CTFPlayer *owner = ToTFPlayer(this->GetOwnerEntity());
	if (owner != nullptr) {
		duration = CAttributeManager::AttribHookValue<int>((int)duration,
			"canteen_specialist", owner, nullptr, true);
	}
	
	// TODO
	
	return true;
}


CTFPowerupBottle::PowerupType CTFPowerupBottle::GetPowerupType() const
{
	if (CAttributeManager::AttribHookValue<int>(0, "critboost",
		this, nullptr, true) != 0) {
		return PowerupType::CRITBOOST;
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "ubercharge",
		this, nullptr, true) != 0) {
		return PowerupType::UBERCHARGE;
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "recall",
		this, nullptr, true) != 0) {
		return PowerupType::RECALL;
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "refill_ammo",
		this, nullptr, true) != 0) {
		return PowerupType::REFILL_AMMO;
	}
	
	if (CAttributeManager::AttribHookValue<int>(0, "building_instant_upgrade",
		this, nullptr, true) != 0) {
		return PowerupType::BUILDING_UPGRADE;
	}
	
	return PowerupType::EMPTY;
}
