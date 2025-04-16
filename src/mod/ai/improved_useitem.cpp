#include "mod.h"
#include "stub/tfweaponbase.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "stub/gamerules.h"
#include "stub/projectiles.h"


namespace Mod::AI::Improved_UseItem
{
	ConVar cvar_debug("sig_ai_improved_useitem_debug", "0", FCVAR_NOTIFY,
		"Mod: debug CTFBotUseItemImproved and descendants");
	
	
	// TODO: figure out why the hell this was made to be Action<CTFBot> rather than IHotplugAction<CTFBot>;
	// ostensibly it should be the latter, however when I make it the latter, the game goes crashy:
	// the stack trace seems to indicate that an EIP --> garbage situation occurs in the m_DestroyedActions.PurgeAndDeleteElements()
	// step of Behavior<CTFBot>::Update, under CTFBotMainAction's dtor and CTFBotTacticalMonitor's dtor
	// (so it's some kind of "our IHotplugAction virtual dtor doesn't interact well with the game code" situation)
	class CTFBotUseItemImproved : public IHotplugAction<CTFBot>
	{
	public:
		enum class State : int
		{
			FIRE, // wait for the weapon switch delay, then use it
			WAIT, // wait for the item to take effect, then finish
		};
		
		CTFBotUseItemImproved(CTFWeaponBase *item) :
			m_hItem(item) {}
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			if (cvar_debug.GetBool()) {
				DevMsg("[%8.3f] CTFBot%s(#%d): OnStart\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			}
			
			this->m_State = State::FIRE;
			
			this->m_flSwitchTime = this->m_hItem->m_flNextPrimaryAttack;
			actor->PushRequiredWeapon(this->m_hItem);
			
			this->m_ctTimeout.Start(10.0f);
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			if (this->m_ctTimeout.IsElapsed()) {
				if (cvar_debug.GetBool()) {
					DevMsg("[%8.3f] CTFBot%s(#%d): Timeout elapsed!\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
				}
				return ActionResult<CTFBot>::Done("Timed out");
			}
			
			switch (this->m_State) {
			
			case State::FIRE:
				if (gpGlobals->curtime >= this->m_flSwitchTime) {
					if (cvar_debug.GetBool()) {
						DevMsg("[%8.3f] CTFBot%s(#%d): Using item now\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
					}
					actor->PressFireButton(1.0f);
					this->m_State = State::WAIT;
				}
				break;
			
			case State::WAIT:
				if (this->IsDone(actor)) {
					if (cvar_debug.GetBool()) {
						DevMsg("[%8.3f] CTFBot%s(#%d): Done using item\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
					}
					return ActionResult<CTFBot>::Done("Item used");
				}
				break;
			
			}
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			if (cvar_debug.GetBool()) {
				DevMsg("[%8.3f] CTFBot%s(#%d): OnEnd\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			}
			
			actor->ReleaseFireButton();
			actor->PopRequiredWeapon();
		}
		
		virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override
		{
			if (cvar_debug.GetBool()) {
				DevMsg("[%8.3f] CTFBot%s(#%d): OnSuspend\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			}
			
			return ActionResult<CTFBot>::Done("Interrupted");
		}
		
	
	protected:
		virtual bool IsDone(CTFBot *actor) = 0;
		
		static bool IsPossible(CTFBot *actor)
		{
			/* the bot cannot actually PressFireButton in these cases */
			if (actor->m_Shared->IsControlStunned())             return false;
			if (actor->m_Shared->IsLoserStateStunned())          return false;
			if (actor->HasAttribute(CTFBot::ATTR_SUPPRESS_FIRE)) return false;
			
			return true;
		}
		
	private:
		CHandle<CTFWeaponBase> m_hItem;
		
		State m_State;
		float m_flSwitchTime;
		
		CountdownTimer m_ctTimeout;
	};
	
	
	class CTFBotUseBuffItem : public CTFBotUseItemImproved
	{
	public:
		CTFBotUseBuffItem(CTFWeaponBase *item) :
			CTFBotUseItemImproved(item) {}
		
		virtual const char *GetName() const override { return "UseBuffItem"; }
		
		static bool IsPossible(CTFBot *actor)
		{
			/* workaround for integer-division bug where, if mult_buff_duration > 10 (technically, if the final adjusted duration > 100 sec),
			 * then CTFPlayerShared::m_bRageDraining will be true, but CTFPlayerShared::m_flRageMeter will stay at 100.0f indefinitely!
			 * (basically: CTFBot::OpportunisticallyUseWeaponAbilities only checks CTFBuffItem::IsFull(), i.e. is m_flRageMeter >= 100.0f;
			 *  what we want is to ALSO check if rage is active via m_bRageDraining, and refuse to use the item if it is currently active) */
			if (actor->m_Shared->m_flRageMeter < 100.0f) return false;
			if (actor->m_Shared->m_bRageDraining)        return false;
			
			return CTFBotUseItemImproved::IsPossible(actor);
		}
		
	private:
		virtual bool IsDone(CTFBot *actor) override
		{
			return actor->m_Shared->m_bRageDraining;
		}
	};
	
	class CTFBotUseThrowableItem : public CTFBotUseItemImproved
	{
	public:
		CTFBotUseThrowableItem(CTFWeaponBase *item) :
			CTFBotUseItemImproved(item) {}
		
		virtual const char *GetName() const override { return "UseThrowableItem"; }
		
	private:
		virtual bool IsDone(CTFBot *actor) override
		{
			return true;
		}
	};

	class CTFBotUseLunchBoxItem : public CTFBotUseItemImproved
	{
	public:
		enum class TauntState : int
		{
			BEFORE, // taunt has not yet begun
			DURING, // taunt is in progress
			AFTER,  // taunt has completed
		};
		
		CTFBotUseLunchBoxItem(CTFWeaponBase *item) :
			CTFBotUseItemImproved(item) {}
		
		virtual const char *GetName() const override { return "UseLunchBoxItem"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			this->m_TauntState = TauntState::BEFORE;
			
			return CTFBotUseItemImproved::OnStart(actor, action);
		}
		
		using CTFBotUseItemImproved::IsPossible;
		
	private:
		virtual bool IsDone(CTFBot *actor) override
		{
			switch (this->m_TauntState) {
			
			case TauntState::BEFORE:
				if (actor->m_Shared->InCond(TF_COND_TAUNTING)) {
					this->m_TauntState = TauntState::DURING;
				}
				return false;
			
			case TauntState::DURING:
				if (!actor->m_Shared->InCond(TF_COND_TAUNTING)) {
					this->m_TauntState = TauntState::AFTER;
				}
				return false;
			
			case TauntState::AFTER:
				return true;
			
			}
			
			// should never happen
			return true;
		}
		
		TauntState m_TauntState;
	};
	
	CEconItemView *item_noisemaker = nullptr;
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBot_OpportunisticallyUseWeaponAbilities)
	{
		
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		CTFBotUseItem *result = reinterpret_cast<CTFBotUseItem *>(DETOUR_MEMBER_CALL());
		if (result != nullptr) {
			CTFWeaponBase *item = result->m_hItem;
			if (item != nullptr) {
				if (item->GetWeaponID() == TF_WEAPON_BUFF_ITEM) {
					delete result;
					
					if (CTFBotUseBuffItem::IsPossible(bot)) {
						return new CTFBotUseBuffItem(item);
					} else {
						return nullptr;
					}
				}
				
				if (item->GetWeaponID() == TF_WEAPON_LUNCHBOX) {
					delete result;
					
					if (CTFBotUseLunchBoxItem::IsPossible(bot)) {
						return new CTFBotUseLunchBoxItem(item);
					} else {
						return nullptr;
					}
				}
			}
		}
		else
		{
			CTFWearable *pActionSlotEntity = bot->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
			if (!bot->ExtAttr()[CTFBot::ExtendedAttr::HOLD_CANTEENS] && pActionSlotEntity  != nullptr) {

				// get the equipped item and see what it is
				CTFPowerupBottle *pPowerupBottle = rtti_cast< CTFPowerupBottle* >( pActionSlotEntity );
				CTFBot *bot_acting = bot;
				if (bot->IsPlayerClass(TF_CLASS_MEDIC)) {
					auto medigun = rtti_cast<CWeaponMedigun *>(bot->GetActiveWeapon());
					if (medigun != nullptr) {
						CTFBot *patient = ToTFBot(medigun->GetHealTarget());
						if (patient != nullptr) {
							
							bot_acting = patient;
						}
					}
				}

				
				const CKnownEntity *threat = bot_acting->GetVisionInterface()->GetPrimaryKnownThreat(false);
				if ( pPowerupBottle  != nullptr && threat != nullptr && threat->GetEntity() != nullptr && threat->IsVisibleRecently() 
					&& bot_acting->GetIntentionInterface()->ShouldAttack(bot_acting->MyNextBotPointer(), threat) == QueryResponse::YES)
				{
					if ( bot_acting->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) || bot_acting->IsLineOfFireClear( threat->GetEntity()->WorldSpaceCenter() ) || 
						bot_acting->IsLineOfFireClear( threat->GetEntity()->GetAbsOrigin() ))
					{
						bot->UseActionSlotItemPressed();
						bot->UseActionSlotItemReleased();
					}
				
				}

				int iNoiseMaker = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(bot, iNoiseMaker, enable_misc2_noisemaker );
				//DevMsg("Has noise maker %d\n",iNoiseMaker);
				if (!bot->ExtAttr()[CTFBot::ExtendedAttr::HOLD_CANTEENS] && iNoiseMaker != 0) {
					
					item_noisemaker = pActionSlotEntity->GetItem();
					bot->UseActionSlotItemPressed();

					item_noisemaker = nullptr;

					bot->UseActionSlotItemReleased();
				}
			}
			/*for ( int w=0; w<MAX_WEAPONS; ++w )
			{
				CTFWeaponBase *weapon = ( CTFWeaponBase * )bot->GetWeapon( w );
				if ( !weapon )
					continue;

				if ( weapon->GetWeaponID() == TF_WEAPON_JAR || weapon->GetWeaponID() == TF_WEAPON_JAR_MILK || weapon->GetWeaponID() == TF_WEAPON_JAR_GAS || weapon->GetWeaponID() == TF_WEAPON_CLEAVER)
				{
					const CKnownEntity *threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(false);
					if ( weapon->HasAmmo() && threat != nullptr && threat->GetEntity() != nullptr && threat->IsVisibleInFOVNow())
					{
						if (CTFBotUseItemImproved::IsPossible(bot)) {
							return new CTFBotUseThrowableItem(weapon);
						} else {
							return nullptr;
						}
					}
				}
				if ( weapon->GetWeaponID() == TF_WEAPON_SPELLBOOK || strcmp(weapon->GetClassname(), "tf_powerup_bottle") == 0)
				{
					if (strcmp(weapon->GetClassname(), "tf_powerup_bottle") == 0) {
						reinterpret_cast<CTFItem *>(this);
					}
					bot->UseActionSlotItemPressed();
					bot->UseActionSlotItemReleased();
				}
			}*/
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void *, CPlayerInventory_GetInventoryItemByItemID, unsigned long long param1, int* itemid)
	{
		
		if (item_noisemaker != nullptr) {
			return item_noisemaker;
		}
		return DETOUR_MEMBER_CALL(param1, itemid);
	}
	std::vector<bool> stack_m_bPlayingMannVsMachine;
	void Quirk_MvM_Pre()
	{
		stack_m_bPlayingMannVsMachine.push_back(TFGameRules()->IsMannVsMachineMode());
		
		TFGameRules()->Set_m_bPlayingMannVsMachine(false);
	}
	void Quirk_MvM_Post()
	{
		assert(!stack_m_bPlayingMannVsMachine.empty());
		
		TFGameRules()->Set_m_bPlayingMannVsMachine(stack_m_bPlayingMannVsMachine.back());
		stack_m_bPlayingMannVsMachine.pop_back();
	}

	DETOUR_DECL_MEMBER(bool, CTFBot_EquipLongRangeWeapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		//Quirk_MvM_Pre();
		
		bool mannvsmachine = TFGameRules()->IsMannVsMachineMode();
		TFGameRules()->Set_m_bPlayingMannVsMachine(false);

		auto result = DETOUR_MEMBER_CALL();

		TFGameRules()->Set_m_bPlayingMannVsMachine(mannvsmachine);
		//Quirk_MvM_Post();
		return result;
	}

	

	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:Improved_UseItem")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBot_OpportunisticallyUseWeaponAbilities, "CTFBot::OpportunisticallyUseWeaponAbilities");
			MOD_ADD_DETOUR_MEMBER(CTFBot_EquipLongRangeWeapon, "CTFBot::EquipLongRangeWeapon");
			MOD_ADD_DETOUR_MEMBER(CPlayerInventory_GetInventoryItemByItemID, "CPlayerInventory::GetInventoryItemByItemID");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_improved_useitem", "0", FCVAR_NOTIFY,
		"Mod: use improved replacement for CTFBotUseItem",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
