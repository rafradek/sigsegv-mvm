#include "mod.h"
#include "link/link.h"
#include "stub/particles.h"
#include "stub/usermessages_sv.h"
#include "stub/tfplayer.h"
#include "mod/common/commands.h"
#include "stub/entities.h"
#include "stub/misc.h"
#include "util/misc.h"
#include "mod/common/text_hud.h"
#include "stub/extraentitydata.h"
#include "mod/etc/sendprop_override.h"
#include "mod/etc/mapentity_additions.h"
#include "util/prop_helper.h"
#include "mod/etc/entity_limit_manager.h"
#include <bit>

namespace Mod::Util::Overlay_Serverside
{
	int spriteLineIgnoreZ = -1;
	int spriteLine = -1;
	CRecipientFilter filter;
	std::vector<CHandle<CPointWorldText>> text_pool_unused;
	std::vector<CHandle<CPointWorldText>> text_pool_used;

	void DrawLine(const Vector &start, const Vector &end, float life, int r, int g, int b, int a, float width, float endWidth, bool ignoreZ) {
		TE_BeamPoints(filter, 0.0f, &start, &end, ignoreZ ? spriteLineIgnoreZ : spriteLine, 0, 0, 0, Max(0.06f, life), width, endWidth, 70, 0.0, r, g, b, a, 10);
	}
	void DrawLine(const Vector &start, const Vector &end, float life, int r, int g, int b, int a, float width, bool ignoreZ) {
		DrawLine(start, end, life, r, g, b, a, width, width, ignoreZ);
		
	}

	void DrawCircle(const Vector &center, const Vector &xAxis, const Vector &yAxis, float radius, float life, int r, int g, int b, int a, float width, bool ignoreZ) {
		const unsigned int nSegments = 16;
		const float flRadStep = (M_PI*2.0f) / (float) nSegments;

		Vector vecLastPosition;
		
		Vector vecStart = center + xAxis * radius;
		Vector vecPosition = vecStart;

		for (int i = 1; i <= nSegments; i++)
		{
			vecLastPosition = vecPosition;

			float flSin, flCos;
			SinCos( flRadStep*i, &flSin, &flCos );
			vecPosition = center + (xAxis * flCos * radius) + (yAxis * flSin * radius);

			DrawLine(vecLastPosition, vecPosition, life, r, g, b, 255, width, ignoreZ);
		}
	}


	void DrawBox(const Vector &origin, const Vector &mins, const Vector &maxs, float life, int r, int g, int b, int a, float width, const QAngle &angle) {
		Vector vertices[] {{mins[0], mins[1], mins[2]}, {maxs[0], mins[1], mins[2]}, {mins[0], maxs[1], mins[2]}, {mins[0], mins[1], maxs[2]}, 
			{maxs[0], maxs[1], mins[2]}, {maxs[0], mins[1], maxs[2]}, {mins[0], maxs[1], maxs[2]}, {maxs[0], maxs[1], maxs[2]}};
			
		std::pair<int, int> lines[] {{0, 1}, {0, 2}, {0, 3}, {7, 4}, {7, 5}, {7, 6}, {1, 4}, {1, 5}, {2, 4}, {2, 6}, {3, 5}, {3, 6}};
		if (angle != vec3_angle) {
			matrix3x4_t matrix;
			AngleMatrix( angle, vec3_origin, matrix );
			for (auto &vertex : vertices) {
				Vector in = vertex;
				VectorTransform(in, matrix, vertex);
				vertex += origin;
			}
		}
		else {
			for (auto &vertex : vertices) {
				vertex += origin;
			}
		}
		for (auto &line : lines) {
			DrawLine(vertices[line.first], vertices[line.second], life, r, g, b, 255, width, a > 0);
		}
	}
	

	void DrawSweptBox(const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, float life, int r, int g, int b, int a, float width, const QAngle &angle) {
		Vector vertices[] {{mins[0], mins[1], mins[2]}, {maxs[0], mins[1], mins[2]}, {mins[0], maxs[1], mins[2]}, {mins[0], mins[1], maxs[2]}, 
			{maxs[0], maxs[1], mins[2]}, {maxs[0], mins[1], maxs[2]}, {mins[0], maxs[1], maxs[2]}, {maxs[0], maxs[1], maxs[2]}};
			
		std::pair<int, int> lines[] {{0, 1}, {0, 2}, {0, 3}, {7, 4}, {7, 5}, {7, 6}, {1, 4}, {1, 5}, {2, 4}, {2, 6}, {3, 5}, {3, 6}};
		if (angle != vec3_angle) {
			for (auto &vertex : vertices) {
				VectorRotate(vertex, angle, vertex);
			}
		}
		for (auto &line : lines) {
			DrawLine(vertices[line.first] + start, vertices[line.second] + start, life, r, g, b, 255, width, false);
			DrawLine(vertices[line.first] + end, vertices[line.second] + end, life, r, g, b, 255, width, false);
		}
		for (auto &vertex : vertices) {
			DrawLine(vertex + start, vertex + end, life, r, g, b, 255, width, false);
		}
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Box, const Vector& origin, const Vector& mins, const Vector& maxs, int r, int g, int b, int a, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawBox(origin, mins, maxs, flDuration, r, g, b, a, 0.28f, vec3_angle);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_BoxDirection, const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& orientation, int r, int g, int b, int a, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		QAngle angles = vec3_angle;
		angles.y = UTIL_VecToYaw(orientation);
		DrawBox(origin, mins, maxs, flDuration, r, g, b, a, 0.28f, angles);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_BoxAngles, const Vector& origin, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawBox(origin, mins, maxs, flDuration, r, g, b, a, 0.28f, angles);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_SweptBox, const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, const QAngle& angles, int r, int g, int b, int a, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawSweptBox(start, end, mins, maxs, flDuration, r, g, b, a, 0.28f, angles);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityBounds, const CBaseEntity *pEntity, int r, int g, int b, int a, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		const CCollisionProperty *pCollide = pEntity->CollisionProp();
		DrawBox(pCollide->GetCollisionOrigin(), pCollide->OBBMins(), pCollide->OBBMaxs(), flDuration, r, g, b, a, 0.28f, pCollide->GetCollisionAngles());
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Line, const Vector& origin, const Vector& target, int r, int g, int b, bool noDepthTest, float duration)
	{
		if (filter.GetRecipientCount() == 0) return;
		
		DrawLine(origin, target, duration, r, g, b, 255, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Triangle, const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawLine(p1, p2, duration, r, g, b, a, 0.28f, noDepthTest);
		DrawLine(p1, p3, duration, r, g, b, a, 0.28f, noDepthTest);
		DrawLine(p2, p3, duration, r, g, b, a, 0.28f, noDepthTest);
	}
	
	
	// DETOUR_DECL_STATIC(void, NDebugOverlay_Grid, const Vector& vPosition)
	// {
	// 	Send_Grid(vPosition);
	// }
	
	THINK_FUNC_DECL(TextRemove)
	{
		auto text = (CPointWorldText*)this;
		text_pool_unused.erase(std::find(text_pool_unused.begin(), text_pool_unused.end(), text));
	}
	
	THINK_FUNC_DECL(TextHide)
	{
		auto text = (CPointWorldText*)this;
		text_pool_unused.push_back(text);
		text_pool_used.erase(std::find(text_pool_used.begin(), text_pool_used.end(), text));
		text->m_szText.Get()[0] = 0;
		text->AddEffects(EF_NODRAW);
	}

	void WorldTextScale(CBaseEntity *entity, int clientIndex, DVariant &value, int callbackIndex, SendProp *prop, uintptr_t data) {
		auto player = UTIL_PlayerByIndex(clientIndex);
		if (player != nullptr) {
			value.m_Float = 3.0f + player->EyePosition().DistTo(entity->GetAbsOrigin()) / 84;
		}
	}

	CPointWorldText *CreateText() {
		CPointWorldText *entity = reinterpret_cast<CPointWorldText *>(CreateEntityByName("point_worldtext"));
		entity->m_nOrientation = 1;
		entity->m_flTextSize = 12;
		entity->m_nFont = 8;
		Vector pos = Vector(0,0,1);
		entity->Teleport(&pos, &vec3_angle, &vec3_origin);
		DispatchSpawn(entity);
		entity->Activate();
		
		auto mod = entity->GetOrCreateEntityModule<Mod::Etc::SendProp_Override::SendpropOverrideModule>("sendpropoverride");
		static SendProp *propTextSize;
		static int indexPropTextSize = FindSendPropPrecalcIndex(entity->GetServerClass()->m_pTable, "m_flTextSize", 0, propTextSize);
		mod->AddOverride(WorldTextScale, indexPropTextSize, propTextSize, 0);
		Mod::Etc::Entity_Limit_Manager::MarkEntityAsDisposable(entity);
		return entity;
	}

	CPointWorldText *FindText() {
		CPointWorldText *text = nullptr;
		Iterate(text_pool_unused, [&](auto handle){
			if (handle != nullptr) {
				text = handle;
				text->RemoveEffects(EF_NODRAW);
				return IT_REMOVE_BREAK;
			}
			else {
				return IT_REMOVE;
			}
		});
		if (text == nullptr) {
			return CreateText();
		}
		return text;
	}

	struct WorldTextEntry {
		Vector pos;
		int linenum = 0;
		std::string text;
		int timeout = 0;
		CHandle<CPointWorldText> entity;
		EHANDLE attach;
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;
		int drawTick = 0;
	};
	std::vector<WorldTextEntry> worldTexts;

	extern ConVar cvar_drawtoall;
	
	void DrawTextEntry(const Vector &pos, CBaseEntity *attachEntity) {
		std::vector<std::string> lines;
		std::string compiledText;
		WorldTextEntry *entryToUse = nullptr;
		for (auto &e : worldTexts) {
			if (e.pos == pos && e.timeout >= gpGlobals->tickcount && e.attach == attachEntity && e.entity != nullptr) {
				if (e.linenum >= lines.size()) {
					lines.resize(e.linenum + 1);
				}
				lines[e.linenum] = e.text;
				if (entryToUse == nullptr || entryToUse->timeout < e.timeout) {
					entryToUse = &e;
				}
				e.drawTick = gpGlobals->tickcount;
			}
		}
		for (auto &line : lines) {
			compiledText += line;
			compiledText += '\n';
		}

		if (entryToUse == nullptr) return;

		CPointWorldText *entity = entryToUse->entity;

		V_strncpy(entity->m_szText.Get(), compiledText.c_str(), 260);
		entity->Teleport(&pos, &vec3_angle, &vec3_origin);
		
		auto modvis = entity->GetOrCreateEntityModule<Mod::Etc::Mapentity_Additions::VisibilityModule>("visibility");
		modvis->defaultHide = !cvar_drawtoall.GetBool();
		if (!cvar_drawtoall.GetBool()) {
			int c = filter.GetRecipientCount();
			for (int i = 0; i < c; i++) {
				modvis->hideTo.push_back(INDEXENT(filter.GetRecipientIndex(i)));
			} 
		}

		text_pool_used.push_back(entity);
		THINK_FUNC_SET(entity, TextHide, TICKS_TO_TIME(entryToUse->timeout));
		THINK_FUNC_SET(entity, TextRemove, TICKS_TO_TIME(entryToUse->timeout) + 0.25f);
	}

	void Text(const char *text, int line, const Vector &origin, float flDuration, CBaseEntity *attachEntity, int r, int g, int b, int a) {
		if (filter.GetRecipientCount() == 0) return;
		Vector pos = origin;
		if (origin == vec3_origin) {
			pos = Vector(0,0,1);
		}

		CHandle<CPointWorldText> textHandle;
		WorldTextEntry *entry = nullptr;
		for (auto &e : worldTexts) {
			if (e.pos == pos && e.attach == attachEntity) {
				textHandle = e.entity;
				if (e.linenum == line) {
					entry = &e;
				}
			}
		}
		
		if (entry == nullptr) {
			entry = &worldTexts.emplace_back();
			entry->pos = pos;
			entry->linenum = line;
		}
		int tickDuration = Max(1,TIME_TO_TICKS(flDuration));
		entry->text = text;
		entry->timeout = Max(gpGlobals->tickcount + tickDuration, entry->timeout);
		
		tickDuration = entry->timeout - gpGlobals->tickcount;
		if (textHandle == nullptr) {
			textHandle = FindText();
		}
		if (attachEntity != nullptr) {
			entry->attach = attachEntity;
		}
		entry->entity = textHandle;
		entry->r = r;
		entry->g = g;
		entry->b = b;
		entry->a = a;
		DrawTextEntry(pos, attachEntity);
	}

	
	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityText, int entityID, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Text(text, text_offset, vec3_origin, flDuration, GetContainingEntity(INDEXENT(entityID)), r, g, b, a);
	}

	DETOUR_DECL_STATIC(void, NDebugOverlay_EntityTextAtPosition, const Vector& origin, int text_offset, const char *text, float flDuration, int r, int g, int b, int a)
	{
		Text(text, text_offset, origin, flDuration, nullptr, r, g, b, a);
	}

	DETOUR_DECL_STATIC(void, NDebugOverlay_Text, const Vector& origin, const char *text, bool bViewCheck, float flDuration)
	{
		Text(text, 0, origin, flDuration, nullptr, 255, 255,255, 255);
	}
	struct ScreenTextEntry {
		float xpos;
		float ypos;
		int linenum = 0;
		std::string text;
		int timeout = 0;
		int id = -1;
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;
		int drawTick = 0;
	};
	std::vector<ScreenTextEntry> screenTexts;

	void DrawScreenTextEntry(float xPos, float yPos) {
		
		std::vector<std::string> lines;
		std::string compiledText;
		ScreenTextEntry *entryToUse = nullptr;
		for (auto &e : screenTexts) {
			if (e.xpos == xPos && e.ypos == yPos && e.timeout >= gpGlobals->tickcount) {
				if (e.linenum >= lines.size()) {
					lines.resize(e.linenum + 1);
				}
				lines[e.linenum] = e.text;
				if (entryToUse == nullptr || entryToUse->timeout < e.timeout) {
					entryToUse = &e;
				}
				e.drawTick = gpGlobals->tickcount;
			}
		}
		for (auto &line : lines) {
			compiledText += line;
			compiledText += '\n';
		}

		if (entryToUse == nullptr) return;

		hudtextparms_t textparam;
		textparam.channel = 3;
		textparam.x = xPos;
		textparam.y = yPos;
		textparam.effect = 0;
		textparam.r1 = entryToUse->r * entryToUse->a/255;
		textparam.r2 = entryToUse->r * entryToUse->a/255;
		textparam.b1 = entryToUse->b * entryToUse->a/255;
		textparam.b2 = entryToUse->b * entryToUse->a/255;
		textparam.g1 = entryToUse->g * entryToUse->a/255;
		textparam.g2 = entryToUse->g * entryToUse->a/255;
		textparam.a1 = entryToUse->a;
		textparam.a2 = entryToUse->a;
		textparam.fadeinTime = 0.f;
		textparam.fadeoutTime = 0.f;
		textparam.holdTime = TICKS_TO_TIME(entryToUse->timeout - gpGlobals->tickcount);
		textparam.fxTime = 1.0f;
		int c = filter.GetRecipientCount(); 
		int id = entryToUse->id;
		for (int i = 0; i < c; i++) {
			auto player = UTIL_PlayerByIndex(filter.GetRecipientIndex(i));
			DisplayHudMessageAutoChannel(player, textparam, compiledText.c_str(), id);
		}
	}
	std::vector<Vector2D> updatedScreenTexts;
	void ScreenText(float flXpos, float flYpos, int line, const char *text, int r, int g, int b, int a, float flDuration) {
		if (filter.GetRecipientCount() == 0) return;

		static int newTextId=3424234;
		int textId = newTextId++;

		ScreenTextEntry *entry = nullptr;
		for (auto &e : screenTexts) {
			if (e.xpos == flXpos && e.ypos == flYpos && e.linenum == line) {
				entry = &e;
			}
			if (e.xpos == flXpos && e.ypos == flYpos) {
				textId = e.id;
			}
		}
		
		if (entry == nullptr) {
			entry = &screenTexts.emplace_back();
			entry->xpos = flXpos;
			entry->ypos = flYpos;
			entry->linenum = line;
		}
		int tickDuration = Max(1,TIME_TO_TICKS(flDuration));
		entry->text = text;
		entry->timeout = Max(gpGlobals->tickcount + tickDuration, entry->timeout);
		
		tickDuration = entry->timeout - gpGlobals->tickcount;
		if (entry->id != -1) {
			textId = entry->id;
		}
		entry->id = textId;
		entry->r = r;
		entry->g = g;
		entry->b = b;
		entry->a = a;
		DrawScreenTextEntry(flXpos, flYpos);
		if (std::find(updatedScreenTexts.begin(), updatedScreenTexts.end(), Vector2D(flXpos, flYpos)) != updatedScreenTexts.end()) {
			updatedScreenTexts.push_back(Vector2D(flXpos, flYpos));
		}
	}

	DETOUR_DECL_STATIC(void, NDebugOverlay_ScreenTextLine, float flXpos, float flYpos, int line, const char *text, int r, int g, int b, int a, float flDuration)
	{
		ScreenText(flXpos, flYpos, line, text, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_ScreenText, float flXpos, float flYpos, const char *text, int r, int g, int b, int a, float flDuration)
	{
		ScreenText(flXpos, flYpos, 0, text, r, g, b, a, flDuration);
		//Send_ScreenText(flXpos, flYpos, text, r, g, b, a, flDuration);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3D_ext, const Vector& position, const Vector& mins, const Vector& maxs, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawLine(position + Vector(mins.x, 0, 0), position + Vector(maxs.x, 0, 0), flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position + Vector(0, mins.y, 0), position + Vector(0, maxs.y, 0), flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position + Vector(0, 0, mins.z), position + Vector(0, 0, maxs.z), flDuration, r, g, b, 255, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3D_size, const Vector& position, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawLine(position - Vector(size, 0, 0), position + Vector(size, 0, 0), flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position - Vector(0, size, 0), position + Vector(0, size, 0), flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position - Vector(0, 0, size), position + Vector(0, 0, size), flDuration, r, g, b, 255, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3DOriented_ang, const Vector& position, const QAngle& angles, float size, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		Vector forward, right, up;
		AngleVectors(angles, &forward, &right, &up);
		DrawLine(position - forward * size, position + forward * size, flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position - right * size, position + right * size, flDuration, r, g, b, 255, 0.28f, noDepthTest);
		DrawLine(position - up * size, position + up * size, flDuration, r, g, b, 255, 0.28f, noDepthTest);
	}

	DETOUR_DECL_STATIC(void, NDebugOverlay_Cross3DOriented_mat, const matrix3x4_t& m, float size, int c, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		Vector forward, left, up, position;
		MatrixGetColumn( m, 0, forward );
		MatrixGetColumn( m, 1, left );
		MatrixGetColumn( m, 2, up );
		MatrixGetColumn( m, 3, position );
		forward *= size;
		left *= size;
		up *= size;

		DrawLine(position - forward, position + forward, flDuration, c, 0, 0, 255, 0.28f, noDepthTest);
		DrawLine(position - left, position + left, flDuration, 0, c, 0, 255, 0.28f, noDepthTest);
		DrawLine(position - up, position + up, flDuration, 0, 0, c, 255, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_HorzArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		DrawLine(startPos, endPos, flDuration, r, g, b, a, 0.28f, noDepthTest);
		DrawLine(endPos - ((endPos - startPos).Normalized()*5), endPos, flDuration, r, g, b, a, 1.35f, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_YawArrow, const Vector& startPos, float yaw, float length, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		Vector endPos = startPos + (UTIL_YawToVector(yaw) * length);
		DrawLine(startPos, endPos, flDuration, r, g, b, a, 0.28f, noDepthTest);
		DrawLine(endPos - ((endPos - startPos).Normalized()*5), endPos, flDuration, r, g, b, a, 1.35f, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_VertArrow, const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;
		
		DrawLine(startPos, endPos, flDuration, r, g, b, a, 0.28f, noDepthTest);
		DrawLine(endPos - ((endPos - startPos).Normalized()*5), endPos, flDuration, r, g, b, a, 1.35f, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Axis, const Vector& position, const QAngle& angles, float size, bool noDepthTest, float flDuration)
	{
		if (filter.GetRecipientCount() == 0) return;

		Vector forward, right, up;
		AngleVectors(angles, &forward, &right, &up);
		forward = position + (size * forward);
		right = position - (size * right); // Left is positive
		up = position + (size * up);

		DrawLine(position, forward, flDuration, 255, 0, 0, 255, 0.28f, noDepthTest);
		DrawLine(position, right, flDuration, 0, 255, 0, 255, 0.28f, noDepthTest);
		DrawLine(position, up, flDuration, 0, 0, 255, 255, 0.28f, noDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Sphere, const Vector& center, float radius, int r, int g, int b, bool noDepthTest, float flDuration)
	{
		matrix3x4_t xform;
		AngleMatrix( vec3_angle, center, xform );
		Vector xAxis, yAxis, zAxis;
		MatrixGetColumn( xform, 0, xAxis );
		MatrixGetColumn( xform, 1, yAxis );
		MatrixGetColumn( xform, 2, zAxis );

		DrawCircle(center, xAxis, yAxis, radius, flDuration, r, g, b, 0, 0.28f, noDepthTest);	// xy plane
		DrawCircle(center, yAxis, zAxis, radius, flDuration, r, g, b, 0, 0.28f, noDepthTest);	// yz plane
		DrawCircle(center, xAxis, zAxis, radius, flDuration, r, g, b, 0, 0.28f, noDepthTest);	// xz plane
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle, const Vector& position, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		matrix3x4_t xform;
		AngleMatrix( vec3_angle, position, xform );
		Vector xAxis, yAxis;

		MatrixGetColumn( xform, 2, xAxis );
		MatrixGetColumn( xform, 1, yAxis );
		DrawCircle(position, xAxis, yAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		matrix3x4_t xform;
		AngleMatrix( angles, position, xform );
		Vector xAxis, yAxis;

		MatrixGetColumn( xform, 2, xAxis );
		MatrixGetColumn( xform, 1, yAxis );
		DrawCircle(position, xAxis, yAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Circle_axes, const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		DrawCircle(position, xAxis, yAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);
	}
	
	DETOUR_DECL_STATIC(void, NDebugOverlay_Sphere_ang, const Vector& position, const QAngle& angles, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		matrix3x4_t xform;
		AngleMatrix( angles, position, xform );
		Vector xAxis, yAxis, zAxis;

		MatrixGetColumn( xform, 0, xAxis );
		MatrixGetColumn( xform, 1, yAxis );
		MatrixGetColumn( xform, 2, zAxis );
		DrawCircle(position, xAxis, yAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);	// xy plane
		DrawCircle(position, yAxis, zAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);	// yz plane
		DrawCircle(position, xAxis, zAxis, radius, flDuration, r, g, b, a, 0.28f, bNoDepthTest);	// xz plane
	}

	// DETOUR_DECL_STATIC(void, clear_debug_overlays, const CCommand& args)
	// {
	// 	DETOUR_STATIC_CALL(args);
		
	// 	//Send_Clear();
	// }
	
	
	RefCount rc_DrawAllDebugOverlays;
	DETOUR_DECL_STATIC(void, DrawAllDebugOverlays)
	{
		SCOPED_INCREMENT(rc_DrawAllDebugOverlays);
		DETOUR_STATIC_CALL();
	}
	
	DETOUR_DECL_MEMBER(bool, IVEngineServer_IsDedicatedServer)
	{
		if (rc_DrawAllDebugOverlays > 0 && filter.GetRecipientCount() != 0) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL();
	}
	

	DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
        DETOUR_MEMBER_CALL(edict, playername);
		if (cvar_drawtoall.GetBool()) {
			filter.AddRecipient(ToBasePlayer(GetContainingEntity(edict)));
		}
    }
	
    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
		filter.RemoveRecipient(player);
        DETOUR_MEMBER_CALL();
        //Msg("Delete player %d %d\n", player->GetTeamNumber(), player);
    }
	
	DETOUR_DECL_STATIC(CBasePlayer *, UTIL_GetListenServerHost)
	{
		if (filter.GetRecipientCount() == 0)
			return nullptr;
		return UTIL_PlayerByIndex(filter.GetRecipientIndex(0));
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_DrawTimedOverlays)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		ExtraEntityData *oldData = entity->m_extraEntityData;
		if (oldData != nullptr) {
			oldData->SetEntityTimedOverlay();
		}
		DETOUR_MEMBER_CALL();
		if (oldData != nullptr) {
			oldData->RestoreExtraEntityData();
		}
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_AddTimedOverlay, const char *msg, int time)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		ExtraEntityData *oldData = entity->m_extraEntityData;
		if (oldData != nullptr) {
			oldData->SetEntityTimedOverlay();
		}
		DETOUR_MEMBER_CALL(msg, time);
		if (oldData != nullptr) {
			oldData->RestoreExtraEntityData();
		}
	}

	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener, IModCallbackListener
	{
	public:
		CMod() : IMod("Util:Overlay_Send")
		{
		//	MOD_ADD_DETOUR_MEMBER(IServerGameDLL_GameFrame, "IServerGameDLL::GameFrame");
			
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Box,                  "NDebugOverlay::Box");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_BoxDirection,         "NDebugOverlay::BoxDirection");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_BoxAngles,            "NDebugOverlay::BoxAngles");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_SweptBox,             "NDebugOverlay::SweptBox");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityBounds,         "NDebugOverlay::EntityBounds");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Line,                 "NDebugOverlay::Line");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Triangle,             "NDebugOverlay::Triangle");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityText,           "NDebugOverlay::EntityText");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_EntityTextAtPosition, "NDebugOverlay::EntityTextAtPosition");
			// MOD_ADD_DETOUR_STATIC(NDebugOverlay_Grid,                 "NDebugOverlay::Grid");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Text,                 "NDebugOverlay::Text");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_ScreenTextLine,       "NDebugOverlay::ScreenTextLine");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_ScreenText,           "NDebugOverlay::ScreenText");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3D_ext,          "NDebugOverlay::Cross3D_ext");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3D_size,         "NDebugOverlay::Cross3D_size");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3DOriented_ang,  "NDebugOverlay::Cross3DOriented_ang");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Cross3DOriented_mat,  "NDebugOverlay::Cross3DOriented_mat");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_HorzArrow,            "NDebugOverlay::HorzArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_YawArrow,             "NDebugOverlay::YawArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_VertArrow,            "NDebugOverlay::VertArrow");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Axis,                 "NDebugOverlay::Axis");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Sphere,               "NDebugOverlay::Sphere");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle,               "NDebugOverlay::Circle");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle_ang,           "NDebugOverlay::Circle_ang");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Circle_axes,          "NDebugOverlay::Circle_axes");
			MOD_ADD_DETOUR_STATIC(NDebugOverlay_Sphere_ang,           "NDebugOverlay::Sphere_ang");
			
			// MOD_ADD_DETOUR_STATIC(clear_debug_overlays, "clear_debug_overlays");
			
			MOD_ADD_DETOUR_STATIC(DrawAllDebugOverlays,             "DrawAllDebugOverlays");
			MOD_ADD_DETOUR_MEMBER(IVEngineServer_IsDedicatedServer, "IVEngineServer::IsDedicatedServer");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateOnRemove, "CTFPlayer::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
			MOD_ADD_DETOUR_STATIC(UTIL_GetListenServerHost, "UTIL_GetListenServerHost");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_DrawTimedOverlays, "CBaseEntity::DrawTimedOverlays");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_AddTimedOverlay, "CBaseEntity::AddTimedOverlay");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			if (filter.GetRecipientCount() == 0) return;
			if (!screenTexts.empty()) {
				RemoveIf(screenTexts, [&](auto &entry) {
					if (entry.timeout < gpGlobals->tickcount) {
						updatedScreenTexts.push_back(Vector2D(entry.xpos, entry.ypos));
						return true;
					}
					return false;
				});
				for (auto &vec : updatedScreenTexts) {
					DrawScreenTextEntry(vec.x, vec.y);
				}
				updatedScreenTexts.clear();
			}
			if (!worldTexts.empty()) {
				RemoveIf(worldTexts, [&](auto &entry) {
					if (entry.timeout < gpGlobals->tickcount) {
						DrawTextEntry(entry.pos, entry.attach);
						return true;
					}
					if (entry.attach.IsValid()) {
						if (entry.attach != nullptr && entry.entity != nullptr) {
							const CCollisionProperty *pCollide = entry.attach->CollisionProp();
							
							entry.entity->SetAbsOrigin(pCollide->GetCollisionOrigin() + Vector(0,0,pCollide->OBBMaxs().z + 20));
						}
					}
					return false;
				});
			}
		}

		virtual void OnEnable() override {
			spriteLine = CBaseEntity::PrecacheModel("materials/cable/pure_white.vmt");
			spriteLineIgnoreZ = CBaseEntity::PrecacheModel("materials/vgui/white.vmt");
		}
		virtual void LevelInitPreEntity() override {
			screenTexts.clear();
			worldTexts.clear();
			if (!cvar_drawtoall.GetBool()) {
				filter.RemoveAllRecipients();
			}
			spriteLine = CBaseEntity::PrecacheModel("materials/cable/pure_white.vmt");
			spriteLineIgnoreZ = CBaseEntity::PrecacheModel("materials/vgui/white.vmt");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_overlay_serverside", "0", FCVAR_NOTIFY,
		"Utility: implement debug overlays on serverside",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	ConVar cvar_drawtoall("sig_util_overlay_serverside_draw_to_all", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Draw debug overlays to all players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			if (cvar_drawtoall.GetBool()) {
				filter.AddAllPlayers();
			}
			else {
				filter.RemoveAllRecipients();
			}
		});
	
	
	ModCommandClientAdmin sig_util_overlay_subscribe("sig_util_overlay_subscribe", [](CCommandPlayer *player, const CCommand &args){
		filter.AddRecipient(player);
		ModCommandResponse("[sig_util_overlay_subscribe] Subscribed to debug overlays");
    }, &s_Mod, "Enable receiving overlays on this client");
	
	ModCommandClientAdmin sig_util_overlay_unsubscribe("sig_util_overlay_unsubscribe", [](CCommandPlayer *player, const CCommand &args){
		filter.RemoveRecipient(player);
		ModCommandResponse("[sig_util_overlay_unsubscribe] Unsubscribed from debug overlays");
    }, &s_Mod, "Enable receiving overlays on this client");
}
