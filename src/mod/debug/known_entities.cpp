#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "stub/objects.h"


namespace Mod::Debug::Known_Entities
{
	// cvar: select bot by index (or maybe just use nb_select to choose)
	
	// use ScreenText to print a table on the client:
	// title: "KNOWN ENTITIES FOR BOT #<entidx> (<playername>)"
	// headers; and one line for each known entity
	// colorize lines by type: one color for bots, another for humans, another for buildings, another for other stuff
	// add another line at the bottom for the key/legend
	
	// default color: light gray
	
	// columns:
	
	// - entindex
	// - name
	//   - bots/players: "<playername>"
	//   - buildings: "<playername>'s <bldgtype>"
	//   - others: "<classname>"
	
	// - ignored [? IsIgnored]
	// - noticed [? IsVisibleEntityNoticed]
	
	// - visibility
	//   - if visible now:
	//     - show time: GetTimeSinceBecameVisible()
	//     - color green if time >= vision->GetMinRecognizeTime()
	//     - color cyan  if time <  vision->GetMinRecognizeTime()
	//   - if not visible now:
	//     - if !WasEverVisible():
	//       - show "never"
	//       - color red
	//     - if WasEverVisible():
	//       - show time: GetTimeSinceLastSeen()
	//       - color yellow if IsVisibleRecently()
	//       - color red    if !IsVisibleRecently()
	
	// - age
	//   - show time: GetTimeSinceBecameKnown()
	
	// - last known
	//   - show time: GetTimeSinceLastKnown()
	//   - color gradient based on [0~10] seconds (10 seconds are obsolete)
	
	// - last known position:
	//   - print vector of GetLastKnownPosition()
	//   - color green if HasLastKnownPositionBeenSeen()
	//   - color red   if HasLastKnownPositionBeenSeen()
	
	// - actual current position:
	//   - print vector of GetEntity()->GetAbsOrigin()
	//   - color green if distance from GetLastKnownPosition() <= tolerance
	//   - color red   if distance from GetLastKnownPosition() >  tolerance
	//   - need tolerance cvar
	
	// - threat ranking:
	//   - show as an integer digit
	//   - treat SelectMoreDangerousThreat as a less-than operator for a sort algorithm
	//   - do a color gradient from red to green based on the relative ranking
	//   - for the known returned by vision->GetPrimaryKnownThreat(true), mark with an asterisk instead of a digit
	
	// - is immediate threat
	//   - show: yes/no, or "n/a" if the bot is in a behavior that overrides SelectMoreDangerousThreat and doesn't have notion of imm threats
	//   - color: white for yes, gray for no, gray for "n/a"
	
	// have sort options:
	// - by entindex
	// - by time when became known
	// - by threat ranking
	
	// be sure to handle known->IsObsolete() true properly (gray out those entries I guess?)
	// idea: keep showing obsolete knowns for a short period after they go obsolete
	
	// TODO: draw a subset of the info ON the entities themselves!
	
	// column for "looking at", which will show a mark if the bot's current LOOK_AT is the entity
	// also handle vector cases of last known position, if possible
	
	// extra columns for spy info
	
	
	constexpr float duration = 1000.0f;
	
	
	INextBot *thebot = nullptr;
	std::map<int, CKnownEntity> knowns;
	std::map<int, float> obsoletes;
	
	std::map<const CKnownEntity *, int> threatranks;
	
	
	ConVar cvar_res_w("sig_debug_known_entities_res_w", "1920", FCVAR_NOTIFY,
		"Overlay screen resolution: width");
	ConVar cvar_res_h("sig_debug_known_entities_res_h", "1080", FCVAR_NOTIFY,
		"Overlay screen resolution: height");
	
	ConVar cvar_text_spacing("sig_debug_known_entities_text_spacing", "13", FCVAR_NOTIFY,
		"Overlay: text line spacing");
	
	ConVar cvar_box_padding("sig_debug_known_entities_box_padding", "3", FCVAR_NOTIFY,
		"Overlay: padding between text and box edge");
	
	void AddScreenText(int x, int y, int line, int maxchars, const Color& color, uint8_t alpha, const char *format, ...)
	{
		y += (line * cvar_text_spacing.GetInt());
		
		x += cvar_box_padding.GetInt();
		y += cvar_box_padding.GetInt();
		
		static char buf[4096];
		int size = sizeof(buf);
		if (maxchars != 0) {
			size = Min(size, (maxchars + 1));
		}
		
		va_list va;
		va_start(va, format);
		V_vsprintf_safe(buf, format, va);
		va_end(va);
		
		NDebugOverlay::ScreenText(
			RemapValClamped(x, 0, cvar_res_w.GetInt(), 0.0f, 1.0f),
			RemapValClamped(y, 0, cvar_res_h.GetInt(), 0.0f, 1.0f),
			buf,
			color.r(), color.g(), color.b(), alpha,
			duration);
	}
	
	
	bool IsThreat(const CKnownEntity *known)
	{
		if (known->IsObsolete())                   return false;
		if (!known->IsVisibleRecently())           return false;
		if (!thebot->IsEnemy(known->GetEntity()))  return false;
		
		IVision *vision = thebot->GetVisionInterface();
		if (known->GetTimeSinceBecameKnown() < vision->GetMinRecognizeTime()) return false;
		if (vision->IsIgnored(known->GetEntity()))                            return false;
		
		return true;
	}
	
	int CompareThreats(const CKnownEntity *k1, const CKnownEntity *k2)
	{
		constexpr int K1_MORE_DANGEROUS = -1;
		constexpr int EQUAL             =  0;
		constexpr int K2_MORE_DANGEROUS =  1;
		
		bool is_threat1 = IsThreat(k1);
		bool is_threat2 = IsThreat(k2);
		
		if (is_threat1 && is_threat2) {
			const CKnownEntity *result = thebot->GetIntentionInterface()->SelectMoreDangerousThreat(thebot, thebot->GetEntity(), k1, k2);
			if (result == k1) return K1_MORE_DANGEROUS;
			if (result == k2) return K2_MORE_DANGEROUS;
		} else if (is_threat1) {
			return K1_MORE_DANGEROUS;
		} else if (is_threat2) {
			return K2_MORE_DANGEROUS;
		}
		
		return EQUAL;
	}
	
	
	bool SortFunc_TFirst(const CKnownEntity *lhs, const CKnownEntity *rhs)
	{
		return lhs->GetTimeSinceBecameKnown() > rhs->GetTimeSinceBecameKnown();
	}
	
	bool SortFunc_TLast(const CKnownEntity *lhs, const CKnownEntity *rhs)
	{
		return lhs->GetTimeSinceLastKnown() < rhs->GetTimeSinceLastKnown();
	}
	
	bool SortFunc_ThreatRank(const CKnownEntity *lhs, const CKnownEntity *rhs)
	{
		int cmp = CompareThreats(lhs, rhs);
		if (cmp != 0) {
			return (cmp < 0);
		} else {
			/* fallback sort for equal threat rank */
			return SortFunc_TFirst(lhs, rhs);
		}
	}
	
	
	ConVar cvar_sort("sig_debug_known_entities_sort", "threat", FCVAR_NOTIFY,
		"Sorting algorithm for known entities: entindex, tfirst, tlast, threat, none");
	
	void SortKnowns(std::vector<const CKnownEntity *>& sorted)
	{
		sorted.clear();
		for (const auto& pair : knowns) {
			sorted.push_back(&pair.second);
		}
		
		const char *method = cvar_sort.GetString();
		if (V_stricmp(method, "entindex") == 0) {
			std::stable_sort(sorted.begin(), sorted.end(), [](const CKnownEntity *lhs, const CKnownEntity *rhs){
				return ENTINDEX(lhs->GetEntity()) < ENTINDEX(rhs->GetEntity());
			});
		} else if (V_stricmp(method, "tfirst") == 0) {
			std::stable_sort(sorted.begin(), sorted.end(), &SortFunc_TFirst);
		} else if (V_stricmp(method, "tlast") == 0) {
			std::stable_sort(sorted.begin(), sorted.end(), &SortFunc_TLast);
		} else if (V_stricmp(method, "threat") == 0) {
			std::stable_sort(sorted.begin(), sorted.end(), &SortFunc_ThreatRank);
		} else if (V_stricmp(method, "none") == 0) {
			/* do nothing */
		} else {
			DevMsg("Unknown sort method \"%s\"\n", method);
		}
	}
	
	
	enum Field
	{
		F_ENTINDEX,
		F_NAME,
		F_TFIRST,
		F_TLAST,
		F_VISIBILITY,
		F_IGNORED,
//		F_NOTICED,
//		F_IMMEDIATE,
		F_THREATRANK,
//		F_KNOWNPOS,
//		F_ACTUALPOS,
		F_SPY,
		
		F_TOTAL,
	};
	
	
	using FieldFunc = void (*)(int, int, int, int, uint8_t, const CKnownEntity *);
	struct FieldInfo
	{
		Field index;
		int width;
		const char *name;
		FieldFunc func;
	};
	
	
	Color black  (0x00, 0x00, 0x00, 0xff);
	Color dkgray (0x20, 0x20, 0x20, 0xff);
	Color ltgray (0xc0, 0xc0, 0xc0, 0xff);
	Color white  (0xff, 0xff, 0xff, 0xff);
	
	Color red    (0xff, 0x40, 0x40, 0xff);
	Color orange (0xff, 0xa0, 0x40, 0xff);
	Color yellow (0xff, 0xff, 0x40, 0xff);
	Color green  (0x40, 0xff, 0x40, 0xff);
	Color cyan   (0x40, 0xff, 0xff, 0xff);
	Color blue   (0x40, 0x40, 0xff, 0xff);
	Color magenta(0xff, 0x40, 0xff, 0xff);
	
	
	ConVar cvar_draw_x("sig_debug_known_entities_draw_x", "800", FCVAR_NOTIFY,
		"Overlay screen position: x");
	ConVar cvar_draw_y("sig_debug_known_entities_draw_y", "100", FCVAR_NOTIFY,
		"Overlay screen position: y");
	
	ConVar cvar_box_w("sig_debug_known_entities_box_w", "800", FCVAR_NOTIFY,
		"Overlay box size: width");
	ConVar cvar_box_h("sig_debug_known_entities_box_h", "600", FCVAR_NOTIFY,
		"Overlay box size: height");
	
	void DrawBox()
	{
		float x = RemapValClamped(cvar_draw_x.GetInt(), 0, cvar_res_w.GetInt(), 0.0f, 1.0f);
		float y = RemapValClamped(cvar_draw_y.GetInt(), 0, cvar_res_h.GetInt(), 0.0f, 1.0f);
		float w = RemapValClamped(cvar_box_w.GetInt(),  0, cvar_res_w.GetInt(), 0.0f, 1.0f);
		float h = RemapValClamped(cvar_box_h.GetInt(),  0, cvar_res_h.GetInt(), 0.0f, 1.0f);
		
		NDebugOverlay::ScreenRect(x, y, x + w, y + h,
			Color(0x00, 0x00, 0x00, 0x80),
			Color(0xff, 0xff, 0xff, 0x80),
			duration);
	}
	
	void DrawTitle(int x, int y, int& line)
	{
		auto bot = rtti_cast<CTFBot *>(thebot);
		if (bot == nullptr) return;
		
		AddScreenText(x, y, line, 0, white, 0xff, "KNOWN ENTITIES FOR BOT #%d \"%s\"", ENTINDEX(bot), bot->GetPlayerName());
		line += 2;
	}
	
	void DrawHeader(int x, int y, int line, const FieldInfo& field)
	{
		constexpr Color& c_default = white;
		constexpr Color& c_sort    = yellow;
		
		Color c = c_default;
		if ((field.index == F_ENTINDEX   && V_stricmp(cvar_sort.GetString(), "entindex") == 0) ||
			(field.index == F_TFIRST     && V_stricmp(cvar_sort.GetString(), "tfirst")   == 0) ||
			(field.index == F_TLAST      && V_stricmp(cvar_sort.GetString(), "tlast")    == 0) ||
			(field.index == F_THREATRANK && V_stricmp(cvar_sort.GetString(), "threat")   == 0)) {
			c = c_sort;
		}
		
		AddScreenText(x, y, line, field.width, c, 0xff, "%s", field.name);
	}
	
	
	void Field_EntIndex(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		AddScreenText(x, y, line, width, white, alpha, "#%d", ENTINDEX(known->GetEntity()));
	}
	
	void Field_Name(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		CBaseEntity *ent = known->GetEntity();
		
		Color c = magenta;
		switch (ent->GetTeamNumber()) {
		case TEAM_UNASSIGNED: c = green;   break;
		case TEAM_SPECTATOR:  c = white;   break;
		case TF_TEAM_RED:     c = red;     break;
		case TF_TEAM_BLUE:    c = blue;    break;
		}
		
		CBasePlayer *player = ToBasePlayer(ent);
		if (player != nullptr) {
			AddScreenText(x, y, line, width, c, alpha, "%s", player->GetPlayerName());
		} else if (ent->IsBaseObject()) {
			auto obj = static_cast<CBaseObject *>(ent);
			
			const char *type = "Object";
			switch (obj->GetType()) {
			case OBJ_DISPENSER:         type = "Dispenser"; break;
			case OBJ_TELEPORTER:        type = (obj->GetObjectMode() == 0 ? "Tele Entrance" : "Tele Exit"); break;
			case OBJ_SENTRYGUN:         type = "Sentry"; break;
			case OBJ_ATTACHMENT_SAPPER: type = "Sapper"; break;
			}
			
			if (obj->GetBuilder() != nullptr) {
				AddScreenText(x, y, line, width, c, alpha, "%s's %s", obj->GetBuilder()->GetPlayerName(), type);
			} else {
				AddScreenText(x, y, line, width, c, alpha, "%s", type);
			}
		} else {
			AddScreenText(x, y, line, width, c, alpha, "%s", ent->GetClassname());
		}
	}
	
	void Field_TFirst(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
//		constexpr Color& c_new = white;
//		constexpr Color& c_old = ltgray;
//		constexpr float t_new = 1.0f;
		
		float dt = known->GetTimeSinceBecameKnown();
		
//		Color c = c_old;
//		if (dt <= t_new) {
//			c[0] = (uint8_t)RemapValClamped(dt, 0.0f, t_new, c_new.r(), c_old.r());
//			c[1] = (uint8_t)RemapValClamped(dt, 0.0f, t_new, c_new.g(), c_old.g());
//			c[2] = (uint8_t)RemapValClamped(dt, 0.0f, t_new, c_new.b(), c_old.b());
//			c[3] = (uint8_t)RemapValClamped(dt, 0.0f, t_new, c_new.a(), c_old.a());
//		}
		
		AddScreenText(x, y, line, width, white, alpha, "%6.1f", dt);
	}
	
	void Field_TLast(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		constexpr Color& c_new = white;
		constexpr Color& c_old = black;
		constexpr float t_obsolete = 10.0f;
		
		float dt = known->GetTimeSinceLastKnown();
		
		Color c;
		c[0] = (uint8_t)RemapValClamped(dt, 0.0f, t_obsolete, c_new.r(), c_old.r());
		c[1] = (uint8_t)RemapValClamped(dt, 0.0f, t_obsolete, c_new.g(), c_old.g());
		c[2] = (uint8_t)RemapValClamped(dt, 0.0f, t_obsolete, c_new.b(), c_old.b());
		c[3] = (uint8_t)RemapValClamped(dt, 0.0f, t_obsolete, c_new.a(), c_old.a());
		
		AddScreenText(x, y, line, width, c, alpha, "%5.1f", dt);
	}
	
	void Field_Visibility(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		constexpr Color& c_now        = green;
		constexpr Color& c_now_rec    = cyan;
		constexpr Color& c_recent     = yellow;
		constexpr Color& c_not_recent = red;
		constexpr Color& c_never      = red;
		
		if (known->IsVisibleInFOVNow()) {
			float dt = known->GetTimeSinceBecameVisible();
			
			if (dt >= thebot->GetVisionInterface()->GetMinRecognizeTime()) {
				AddScreenText(x, y, line, width, c_now, alpha, "visible now");
			} else {
				AddScreenText(x, y, line, width, c_now_rec, alpha, "recognizing...");
			}
		} else {
			if (known->WasEverVisible()) {
				if (known->IsVisibleRecently()) {
					AddScreenText(x, y, line, width, c_recent, alpha, "recently seen");
				} else {
					AddScreenText(x, y, line, width, c_not_recent, alpha, "not visible");
				}
			} else {
				AddScreenText(x, y, line, width, c_never, alpha, "never seen");
			}
		}
	}
	
	void Field_Ignored(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		constexpr Color& c_yes = magenta;
		constexpr Color& c_no  = white;
		
		bool is_ignored = thebot->GetVisionInterface()->IsIgnored(known->GetEntity());
		
		if (is_ignored) {
			AddScreenText(x, y, line, width, c_yes, alpha, "ignored");
		} else {
			AddScreenText(x, y, line, width, c_no, alpha, "no");
		}
	}
	
	void Field_Noticed(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		constexpr Color& c_yes = white;
		constexpr Color& c_no  = magenta;
		
		bool is_noticed = thebot->GetVisionInterface()->IsVisibleEntityNoticed(known->GetEntity());
		
		if (is_noticed) {
			AddScreenText(x, y, line, width, c_yes, alpha, "yes");
		} else {
			AddScreenText(x, y, line, width, c_no, alpha, "NO");
		}
	}
	
	void Field_Immediate(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		constexpr Color& c_notapplicable = ltgray;
		constexpr Color& c_yes           = yellow;
		constexpr Color& c_no            = white;
		
		// n/a in CTFBotSniperLurk
		// n/a in CTFBotSpyAttack
		// different in CTFBotSniperAttack
		
		bool has_CTFBotMainAction   = false;
		bool has_CTFBotSniperAttack = false;
		bool has_CTFBotSniperLurk   = false;
		bool has_CTFBotSpyAttack    = false;
		
		auto behavior = thebot->GetIntentionInterface()->FirstContainedResponder();
		
		for (auto action = static_cast<Action<CTFBot> *>(behavior->FirstContainedResponder());
			action != nullptr; action = static_cast<Action<CTFBot> *>(action->FirstContainedResponder())) {
			if (strcmp(action->GetName(), "MainAction") == 0) {
				has_CTFBotMainAction = true;
			} else if (strcmp(action->GetName(), "SniperAttack") == 0) {
				has_CTFBotSniperAttack = true;
			} else if (strcmp(action->GetName(), "SniperLurk") == 0) {
				has_CTFBotSniperLurk = true;
			} else if (strcmp(action->GetName(), "SpyAttack") == 0) {
				has_CTFBotSpyAttack = true;
			}
		}
		
		if (has_CTFBotSniperAttack || has_CTFBotSniperLurk || has_CTFBotSpyAttack) {
			AddScreenText(x, y, line, width, c_notapplicable, alpha, "n/a");
		} else {
			// TODO
		}
	}
	
	void Field_ThreatRank(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		auto it = threatranks.find(known);
		if (it != threatranks.end()) {
			int rank = (*it).second;
			AddScreenText(x, y, line, width, white, alpha, "#%d", rank);
		} else {
			const char *why = "n/a";
			if (known->IsObsolete()) {
				why = "n/a (obsolete)";
			} else if (!thebot->IsEnemy(known->GetEntity())) {
				why = "n/a (not an enemy)";
			} else if (thebot->GetVisionInterface()->IsIgnored(known->GetEntity())) {
				why = "n/a (ignored)";
			} else if (!known->IsVisibleRecently()) {
				why = "n/a (not seen recently)";
			} else if (known->GetTimeSinceBecameKnown() < thebot->GetVisionInterface()->GetMinRecognizeTime()) {
				why = "n/a (recognizing)";
			}
			
			AddScreenText(x, y, line, width, ltgray, alpha, why);
		}
	}
	
	void Field_KnownPos(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		const Vector *pos = known->GetLastKnownPosition();
		AddScreenText(x, y, line, width, white, alpha, "(%+5.0f %+5.0f %+5.0f)", pos->x, pos->y, pos->z);
	}
	
	void Field_ActualPos(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		const Vector& pos = known->GetEntity()->GetAbsOrigin();
		AddScreenText(x, y, line, width, white, alpha, "(%+5.0f %+5.0f %+5.0f)", pos.x, pos.y, pos.z);
	}
	
	void Field_Spy(int x, int y, int line, int width, uint8_t alpha, const CKnownEntity *known)
	{
		auto bot = rtti_cast<CTFBot *>(thebot);
		if (bot == nullptr) return;
		
		CTFPlayer *player = ToTFPlayer(known->GetEntity());
		if (player != nullptr) {
			if (bot->IsKnownSpy(player)) {
				AddScreenText(x, y, line, width, red, alpha, "KNOWN");
			} else {
				CTFBot::SuspectedSpyInfo_t *info = bot->IsSuspectedSpy(player);
				if (info != nullptr && info->IsCurrentlySuspected()) {
					AddScreenText(x, y, line, width, yellow, alpha, "suspected");
				} else {
					AddScreenText(x, y, line, width, ltgray, alpha, "unknown");
				}
			}
		} else {
			AddScreenText(x, y, line, width, dkgray, alpha, "n/a");
		}
	}
	
	
	constexpr FieldInfo fields[F_TOTAL] = {
		{ F_ENTINDEX,   6,  "Entity",      &Field_EntIndex   },
		{ F_NAME,       25, "Name",        &Field_Name       },
		{ F_TFIRST,     6,  "TFirst",      &Field_TFirst     },
		{ F_TLAST,      5,  "TLast",       &Field_TLast      },
		{ F_VISIBILITY, 14, "Visibility",  &Field_Visibility },
		{ F_IGNORED,    7,  "Ignored",     &Field_Ignored    },
//		{ F_NOTICED,    7,  "Noticed",     &Field_Noticed    },
//		{ F_IMMEDIATE,  3,  "Imm",         &Field_Immediate  },
		{ F_THREATRANK, 23, "Threat Rank", &Field_ThreatRank },
//		{ F_KNOWNPOS,   19, "KnownPos",    &Field_KnownPos   },
//		{ F_ACTUALPOS,  19, "ActualPos",   &Field_ActualPos  },
		{ F_SPY,        9,  "Spy",         &Field_Spy        },
	};
	
	
	void DrawRow(int x, int y, int& line, const CKnownEntity *known)
	{
//		if (known != nullptr) {
//			DevMsg("DrawRow(x = %d, y = %d, line = %d, known = %08x, known->GetEntity() = %08x\n",
//				x, y, line, known, known->GetEntity());
//		}
		
		for (int i = 0; i < F_TOTAL; ++i) {
			if (known == nullptr) {
				DrawHeader(x, y, line, fields[i]);
			} else {
				if (known->GetEntity() != nullptr) {
					constexpr float t_new = 2.0f;
					constexpr float t_old = 10.0f;
					constexpr uint8_t a_new = 0xff;
					constexpr uint8_t a_old = 0x80;
					
					uint8_t alpha = RemapValClamped(known->GetTimeSinceLastKnown(), t_new, t_old, a_new, a_old);
					(*fields[i].func)(x, y, line, fields[i].width, alpha, known);
				}
			}
			
			x += 7 * (fields[i].width + 2);
		}
		
		++line;
	}
	
	
	void DrawOverlay()
	{
		NDebugOverlay::Clear();
		
	//	NDebugOverlay::ScreenLine(0.1f, -0.1f, 0.9f, -0.1f, Color(0xff, 0xff, 0xff, 0xff), duration);
		
	//	NDebugOverlay::ScreenLine(0.2f, 0.2f, 0.8f, 0.8f, Color(0xff, 0xff, 0xff, 0xff), duration);
	//	NDebugOverlay::ScreenLine(0.0f, 1.0f, 1.0f, 0.0f, Color(0xff, 0xff, 0xff, 0x00), Color(0x00, 0xff, 0x00, 0xff), duration);
		
		int x = cvar_draw_x.GetInt();
		int y = cvar_draw_y.GetInt();
		int line = 0;
		
	//	DrawBox();
		
		DrawTitle(x, y, line);
		DrawRow(x, y, line, nullptr);
		
		std::vector<const CKnownEntity *> sorted;
		SortKnowns(sorted);
		for (auto known : sorted) {
			DrawRow(x, y, line, known);
		}
		
		for (const auto& pair : threatranks) {
			char buf[16];
			snprintf(buf, sizeof(buf), "Threat #%d", pair.second);
			
			NDebugOverlay::EntityText(ENTINDEX(pair.first->GetEntity()), 0, buf, duration, 0xff, 0xff, 0xff, 0xff);
		}
	}
	
	void ClearOverlay()
	{
		NDebugOverlay::Clear();
	}
	
	
	void UpdateThreatRanks()
	{
		threatranks.clear();
		
		std::vector<const CKnownEntity *> threats;
		for (const auto& pair : knowns) {
			const CKnownEntity *known = &pair.second;
			
			if (IsThreat(known)) {
				threats.push_back(known);
			}
		}
		
		std::stable_sort(threats.begin(), threats.end(), &SortFunc_ThreatRank);
		
		int count = threats.size();
		for (int i = 0; i < count; ++i) {
			threatranks[threats[i]] = (i + 1);
		}
	}
	
	
	ConVar cvar_obsolete_time("sig_debug_known_entities_obsolete_time", "0.0", FCVAR_NOTIFY,
		"How long to keep track of obsolete known entities");
	
	void UpdateData()
	{
		/* update existing knowns and add new knowns */
		CUtlVector<CKnownEntity> updated;
		thebot->GetVisionInterface()->CollectKnownEntities(&updated);
		FOR_EACH_VEC(updated, i) {
			const CKnownEntity& known = updated[i];
			
			if (known.GetEntity() != nullptr && known.GetEntity() != thebot->GetEntity()) {
				int idx = ENTINDEX(known.GetEntity());
				
				auto it = knowns.find(idx);
				if (it == knowns.end()) {
					/* new entry: copy-construct */
					knowns.emplace(std::make_pair(idx, known));
					DevMsg("[%8.3f] +Known #%d\n", gpGlobals->curtime, idx);
				} else {
					/* existing entry: assignment copy */
					(*it).second = known;
				}
			}
		}
		
		/* schedule obsolete knowns for delayed culling */
		for (const auto& pair : knowns) {
			int idx                   = pair.first;
			const CKnownEntity& known = pair.second;
			
			if (known.IsObsolete()) {
				if (obsoletes.find(idx) == obsoletes.end()) {
					DevMsg("[%8.3f] +Obsolete #%d\n", gpGlobals->curtime, idx);
					obsoletes.emplace(std::make_pair(idx, gpGlobals->curtime));
				}
			} else {
				if (obsoletes.find(idx) != obsoletes.end()) {
					DevMsg("[%8.3f] -Obsolete #%d\n", gpGlobals->curtime, idx);
				}
				obsoletes.erase(idx);
			}
		}
		
		/* cull expired obsolete knowns */
		std::vector<int> cull;
		for (const auto& pair : obsoletes) {
			if (pair.second + cvar_obsolete_time.GetFloat() <= gpGlobals->curtime) {
				cull.push_back(pair.first);
			}
		}
		for (auto idx : cull) {
			knowns.erase(idx);
			obsoletes.erase(idx);
			DevMsg("[%8.3f] -Known #%d\n", gpGlobals->curtime, idx);
		}
		
	//	DevMsg("%u knowns\n", knowns.size());
	//	for (const auto& pair : knowns) {
	//		DevMsg("  #%d => %08x (%08x)\n", pair.first, (uintptr_t)&pair.second, (uintptr_t)pair.second.GetEntity());
	//	}
	//	
	//	DevMsg("%u obsoletes\n", obsoletes.size());
	//	for (const auto& pair : obsoletes) {
	//		DevMsg("  #%d => %f\n", pair.first, pair.second);
	//	}
	}
	
	void ResetData()
	{
		knowns.clear();
		obsoletes.clear();
	}
	
	
	DETOUR_DECL_MEMBER(void, INextBot_Update)
	{
		DETOUR_MEMBER_CALL();
		
		INextBot *selected = TheNextBots().GetSelectedBot();
		if (thebot != selected) {
			thebot = selected;
			ResetData();
			
			if (thebot == nullptr) {
				ClearOverlay();
			}
		}
		
		auto nextbot = reinterpret_cast<INextBot *>(this);
		if (nextbot == thebot) {
			UpdateThreatRanks();
			DrawOverlay();
		}
	}
	
	DETOUR_DECL_MEMBER(void, IVision_Update)
	{
		DETOUR_MEMBER_CALL();
		
		INextBot *nextbot = reinterpret_cast<IVision *>(this)->GetBot();
		if (nextbot != nullptr && nextbot == thebot) {
			UpdateData();
		}
	}
	
	
	DETOUR_DECL_MEMBER(void, IVision_AddKnownEntity, CBaseEntity *ent)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		if (vision->GetBot() == thebot) {
			CTFBot *bot = ToTFBot(vision->GetBot()->GetEntity());
			if (bot != nullptr) {
				Msg("[time %8.3f] [bot #%-2d] IVision::AddKnownEntity [ent #%-2d \"%s\"]\n", gpGlobals->curtime,
					ENTINDEX(bot), ENTINDEX(ent),
					(ent != nullptr ? (ent->IsPlayer() ? static_cast<CBasePlayer *>(ent)->GetPlayerName() : ent->GetClassname()) : "nullptr"));
			}
		}
		
		DETOUR_MEMBER_CALL(ent);
	}
	
	DETOUR_DECL_MEMBER(void, IVision_ForgetEntity, CBaseEntity *ent)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		if (vision->GetBot() == thebot) {
			CTFBot *bot = ToTFBot(vision->GetBot()->GetEntity());
			if (bot != nullptr) {
				Msg("[time %8.3f] [bot #%-2d] IVision::ForgetEntity [ent #%-2d \"%s\"]\n", gpGlobals->curtime,
					ENTINDEX(bot), ENTINDEX(ent),
					(ent != nullptr ? (ent->IsPlayer() ? static_cast<CBasePlayer *>(ent)->GetPlayerName() : ent->GetClassname()) : "nullptr"));
			}
		}
		
		DETOUR_MEMBER_CALL(ent);
	}
	
	DETOUR_DECL_MEMBER(void, IVision_ForgetAllKnownEntities)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		if (vision->GetBot() == thebot) {
			CTFBot *bot = ToTFBot(vision->GetBot()->GetEntity());
			if (bot != nullptr) {
				Msg("[time %8.3f] [bot #%-2d] IVision::ForgetAllKnownEntities\n", gpGlobals->curtime, ENTINDEX(bot));
			}
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBot_IsSuspectedSpy, CTFPlayer *spy)
	{
		// caller: CTFBotVision::IsVisibleEntityNoticed
		
		return DETOUR_MEMBER_CALL(spy);
	}
	DETOUR_DECL_MEMBER(void, CTFBot_SuspectSpy, CTFPlayer *spy)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			Msg("[time %8.3f] [bot #%-2d] CTFBot::SuspectSpy [spy #%-2d \"%s\"]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(spy), (spy != nullptr ? spy->GetPlayerName() : "nullptr"));
		}
		
		// caller: CTFBot::Touch, while stealthed/disguised (MvM)
		
		DETOUR_MEMBER_CALL(spy);
	}
	DETOUR_DECL_MEMBER(void, CTFBot_StopSuspectingSpy, CTFPlayer *spy)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			Msg("[time %8.3f] [bot #%-2d] CTFBot::StopSuspectingSpy [spy #%-2d \"%s\"]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(spy), (spy != nullptr ? spy->GetPlayerName() : "nullptr"));
		}
		
		DETOUR_MEMBER_CALL(spy);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBot_IsKnownSpy, CTFPlayer *spy)
	{
		// caller: CTFBotVision::IsIgnored
		// caller: CTFBotVision::IsVisibleEntityNoticed
		
		return DETOUR_MEMBER_CALL(spy);
	}
	DETOUR_DECL_MEMBER(void, CTFBot_RealizeSpy, CTFPlayer *spy)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			Msg("[time %8.3f] [bot #%-2d] CTFBot::RealizeSpy [spy #%-2d \"%s\"]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(spy), (spy != nullptr ? spy->GetPlayerName() : "nullptr"));
		}
		
		// caller: CTFBot::Touch, while stealthed/disguised (PvP)
		// caller: CTFBot::UpdateDelayedThreatNotices
		// caller: CTFBotVision::IsVisibleEntityNoticed, while disguising
		// caller: CTFBotVision::IsVisibleEntityNoticed, if burning/jarated/stealthblink/bleeding
		
		DETOUR_MEMBER_CALL(spy);
	}
	DETOUR_DECL_MEMBER(void, CTFBot_ForgetSpy, CTFPlayer *spy)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			Msg("[time %8.3f] [bot #%-2d] CTFBot::ForgetSpy [spy #%-2d \"%s\"]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(spy), (spy != nullptr ? spy->GetPlayerName() : "nullptr"));
		}
		
		// caller: CTFBotMainAction::OnOtherKilled
		// caller: CTFBotVision::IsVisibleEntityNoticed, if >75% stealthed
		// caller: CTFBotVision::Update, if not visible recently and disguising
		
		DETOUR_MEMBER_CALL(spy);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_SuspectedSpyInfo_t_Suspect)
	{
		DETOUR_MEMBER_CALL();
	}
	DETOUR_DECL_MEMBER(bool, CTFBot_SuspectedSpyInfo_t_IsCurrentlySuspected)
	{
		return DETOUR_MEMBER_CALL();
	}
	DETOUR_DECL_MEMBER(bool, CTFBot_SuspectedSpyInfo_t_TestForRealizing)
	{
		return DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_DelayedThreatNotice, CHandle<CBaseEntity> ent, float delay)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			const char *str_ent = "nullptr";
			if (ent != nullptr) {
				if (ent->IsPlayer()) {
					str_ent = static_cast<CBasePlayer *>((CBaseEntity *)ent)->GetPlayerName();
				} else {
					str_ent = ent->GetClassname();
				}
			}
			
			Msg("[time %8.3f] [bot #%-2d] CTFBot::DelayedThreatNotice [ent #%-4d \"%s\"] [delay %.3f]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(ent), str_ent, delay);
			
			// caller: CTFPlayer::CheckBlockBackstab, backstabber, 0.5 seconds
			// caller: CTFBotMainAction::OnInjured, if TF_DMG_CUSTOM_BACKSTAB, inflictor, 0.5 seconds
			// caller: CTFBotMainAction::OnInjured, if TF_DMG_CUSTOM_BACKSTAB, inflictor, 0.5 seconds, for non-victim bots based on tf_bot_notice_backstab_*
			// caller: CTFBotMainAction::OnInjured, if DMG_CRITICAL|DMG_BURN, attacker, 0.5 seconds, based on tf_bot_notice_backstab_max_range
			
		//	CTFPlayer *player = ToTFPlayer(ent);
		//	if (player != nullptr) {
		//		auto bot = reinterpret_cast<CTFBot *>(this);
		//		
		//		DevMsg("CTFBot::DelayedThreatNotice(#%d): #%d, %.1f sec delay\n",
		//			ENTINDEX(bot), ENTINDEX(ent), delay);
		//	}
		}
		
		DETOUR_MEMBER_CALL(ent, delay);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_OnWeaponFired, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rtti_cast<INextBot *>(bot) == thebot) {
			const char *str_who = "nullptr";
			if (who != nullptr) {
				if (who->IsPlayer()) {
					str_who = static_cast<CBasePlayer *>(who)->GetPlayerName();
				} else {
					str_who = who->GetClassname();
				}
			}
			
			const char *str_weapon = "nullptr";
			if (weapon != nullptr) {
				str_weapon = weapon->GetClassname();
			}
			
			Msg("[time %8.3f] [bot #%-2d] CTFBot::OnWeaponFired [who #%-4d \"%s\"] [weapon #%-4d \"%s\"]\n", gpGlobals->curtime,
				ENTINDEX(bot), ENTINDEX(who), str_who, ENTINDEX(weapon), str_weapon);
		}
		
		DETOUR_MEMBER_CALL(who, weapon);
	}
	
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMainAction_OnInjured, CTFBot *actor, const CTakeDamageInfo& info)
	{
	//	if (rtti_cast<INextBot *>(actor) == thebot) {
	//		DevMsg("CTFBotMainAction::OnInjured(#%d): attacker #%d, damage bits %08x\n",
	//			ENTINDEX(actor), ENTINDEX(info.GetAttacker()), info.GetDamageType());
	//	}
		
		return DETOUR_MEMBER_CALL(actor, info);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Known_Entities")
		{
			MOD_ADD_DETOUR_MEMBER(INextBot_Update, "INextBot::Update");
			MOD_ADD_DETOUR_MEMBER(IVision_Update,  "IVision::Update");
			
			MOD_ADD_DETOUR_MEMBER(IVision_AddKnownEntity,         "IVision::AddKnownEntity");
			MOD_ADD_DETOUR_MEMBER(IVision_ForgetEntity,           "IVision::ForgetEntity");
			MOD_ADD_DETOUR_MEMBER(IVision_ForgetAllKnownEntities, "IVision::ForgetAllKnownEntities");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_IsSuspectedSpy,    "CTFBot::IsSuspectedSpy");
			MOD_ADD_DETOUR_MEMBER(CTFBot_SuspectSpy,        "CTFBot::SuspectSpy");
			MOD_ADD_DETOUR_MEMBER(CTFBot_StopSuspectingSpy, "CTFBot::StopSuspectingSpy");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_IsKnownSpy, "CTFBot::IsKnownSpy");
			MOD_ADD_DETOUR_MEMBER(CTFBot_RealizeSpy, "CTFBot::RealizeSpy");
			MOD_ADD_DETOUR_MEMBER(CTFBot_ForgetSpy,  "CTFBot::ForgetSpy");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_SuspectedSpyInfo_t_Suspect,              "CTFBot::SuspectedSpyInfo_t::Suspect");
			MOD_ADD_DETOUR_MEMBER(CTFBot_SuspectedSpyInfo_t_IsCurrentlySuspected, "CTFBot::SuspectedSpyInfo_t::IsCurrentlySuspected");
			MOD_ADD_DETOUR_MEMBER(CTFBot_SuspectedSpyInfo_t_TestForRealizing,     "CTFBot::SuspectedSpyInfo_t::TestForRealizing");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_DelayedThreatNotice, "CTFBot::DelayedThreatNotice");
			
			MOD_ADD_DETOUR_MEMBER(CTFBot_OnWeaponFired, "CTFBot::OnWeaponFired");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnInjured, "CTFBotMainAction::OnInjured");
		}
		
		virtual void OnEnable() override
		{
			ResetData();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_known_entities", "0", FCVAR_NOTIFY,
		"Debug: show detailed information about a bot's known entities",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


/* spy stuff:

TODO: finish up decompiling the spy related functions in mvm-reversed

Also do the stuff for delayed (threat) notices

Then, find all of the callers of each of the spy/notification funcs


Come up with a way to display this info usefully


1. Integrate the basics into current table (have a column for whether the known is a unknown/suspected/known spy)
2. Have another table, where a human player is singled out, and then it shows all of the bots and what each of them knows about the enemy spy
   (e.g. show each of the incoming events that make them suspect you, realize you, etc)

*/

/*

figure out all of the ways in which you can *become* known to bots in any way
and find a visual way to represent all of those possible transitions in the overlay
(e.g. make it apparent that by firing your weapon, you became known, etc)

*/
