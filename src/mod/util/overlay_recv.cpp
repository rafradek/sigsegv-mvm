#include "mod.h"
#include "mod/util/overlay_shared.h"
#include "stub/usermessages_cl.h"
#include "util/float16.h"
#include "util/socket_old.h"
#include "stub/cdebugoverlay.h"


namespace Mod::Util::Overlay_Recv
{
	ConVar cvar_trace("sig_util_overlay_recv_trace", "0", FCVAR_NOTIFY,
		"Trace overlay messages as they are received");
	
	ConVar cvar_sphere_ntheta("sig_util_overlay_recv_ntheta", "16", FCVAR_NOTIFY,
		"Set nTheta value for CDebugOverlay::AddSphereOverlay");
	ConVar cvar_sphere_nphi("sig_util_overlay_recv_nphi", "16", FCVAR_NOTIFY,
		"Set nPhi value for CDebugOverlay::AddSphereOverlay");
	
	
	void Box(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		Vector mins;   msg.ReadBitVec3Coord(mins);
		Vector maxs;   msg.ReadBitVec3Coord(maxs);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Box] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x%02x %.2f\n",
				origin.x, origin.y, origin.z,
				mins.x, mins.y, mins.z,
				maxs.x, maxs.y, maxs.z,
				r, g, b, a,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddBoxOverlay(origin, mins, maxs, vec3_angle, r, g, b, a, flDuration);
		}
	}
	
	void BoxAngles(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		Vector mins;   msg.ReadBitVec3Coord(mins);
		Vector maxs;   msg.ReadBitVec3Coord(maxs);
		QAngle angles; msg.ReadBitAngles(angles);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[BoxAngles] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x%02x %.2f\n",
				origin.x, origin.y, origin.z,
				mins.x, mins.y, mins.z,
				maxs.x, maxs.y, maxs.z,
				angles.x, angles.y, angles.z,
				r, g, b, a,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	
	void SweptBox(bf_read& msg)
	{
		Vector start;  msg.ReadBitVec3Coord(start);
		Vector end;    msg.ReadBitVec3Coord(end);
		Vector mins;   msg.ReadBitVec3Coord(mins);
		Vector maxs;   msg.ReadBitVec3Coord(maxs);
		QAngle angles; msg.ReadBitAngles(angles);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[SweptBox] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x%02x %.2f\n",
				start.x, start.y, start.z,
				end.x, end.y, end.z,
				mins.x, mins.y, mins.z,
				maxs.x, maxs.y, maxs.z,
				angles.x, angles.y, angles.z,
				r, g, b, a,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddSweptBoxOverlay(start, end, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	
	void Line(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		Vector target; msg.ReadBitVec3Coord(target);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float duration; _float32(&duration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Line] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x %d %.2f\n",
				origin.x, origin.y, origin.z,
				target.x, target.y, target.z,
				r, g, b,
				noDepthTest,
				duration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddLineOverlay(origin, target, r, g, b, noDepthTest, duration);
		}
	}
	
	void Triangle(bf_read& msg)
	{
		Vector p1; msg.ReadBitVec3Coord(p1);
		Vector p2; msg.ReadBitVec3Coord(p2);
		Vector p3; msg.ReadBitVec3Coord(p3);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float duration; _float32(&duration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Triangle] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x%02x %d %.2f\n",
				p1.x, p1.y, p1.z,
				p2.x, p2.y, p2.z,
				p3.x, p3.y, p3.z,
				r, g, b, a,
				noDepthTest,
				duration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddTriangleOverlay(p1, p2, p3, r, g, b, a, noDepthTest, duration);
		}
	}
	
	void EntityText(bf_read& msg)
	{
		int entityID = msg.ReadUBitLong(ENTITY_ID_BITS);
		int text_offset = msg.ReadChar();
		const char *text = msg.ReadAndAllocateString();
		float duration; _float32(&duration, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		
		if (cvar_trace.GetBool()) {
			DevMsg("[EntityText] #%d %d \"%s\" %.2f %02x%02x%02x%02x\n",
				entityID,
				text_offset,
				text,
				duration,
				r, g, b, a);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddEntityTextOverlay(entityID, text_offset, duration, r, g, b, a, "%s", text);
		}
		
		delete[] text;
	}
	
	void EntityTextAtPosition(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		int text_offset = msg.ReadChar();
		const char *text = msg.ReadAndAllocateString();
		float duration; _float32(&duration, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		
		if (cvar_trace.GetBool()) {
			DevMsg("[EntityTextAtPosition] <%+4.0f %+4.0f %+4.0f> %d \"%s\" %.2f %02x%02x%02x%02x\n",
				origin.x, origin.y, origin.z,
				text_offset,
				text,
				duration,
				r, g, b, a);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddTextOverlayRGB(origin, text_offset, duration, r, g, b, a, "%s", text);
		}
		
		delete[] text;
	}
	
	void Grid(bf_read& msg)
	{
		Vector vPosition; msg.ReadBitVec3Coord(vPosition);
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Grid] <%+4.0f %+4.0f %+4.0f>\n",
				vPosition.x, vPosition.y, vPosition.z);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddGridOverlay(vPosition);
		}
	}
	
	void Text(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		const char *text = msg.ReadAndAllocateString();
		bool bViewCheck = !!msg.ReadOneBit();
		float duration; _float32(&duration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Text] <%+4.0f %+4.0f %+4.0f> \"%s\" %d %.2f\n",
				origin.x, origin.y, origin.z,
				text,
				bViewCheck,
				duration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddTextOverlay(origin, duration, "%s", text);
		}
		
		delete[] text;
	}
	
	void ScreenText(bf_read& msg)
	{
		float flXpos; _float32(&flXpos, msg.ReadWord());
		float flYpos; _float32(&flYpos, msg.ReadWord());
		const char *text = msg.ReadAndAllocateString();
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		float duration; _float32(&duration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[ScreenText] %5.1f %5.1f \"%s\" %02x%02x%02x%02x %.2f\n",
				flXpos, flYpos,
				text,
				r, g, b, a,
				duration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddScreenTextOverlay(flXpos, flYpos, duration, r, g, b, a, text);
		}
		
		delete[] text;
	}
	
	void Cross3D_ext(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		Vector mins;     msg.ReadBitVec3Coord(mins);
		Vector maxs;     msg.ReadBitVec3Coord(maxs);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Cross3D_ext] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				mins.x, mins.y, mins.z,
				maxs.x, maxs.y, maxs.z,
				r, g, b,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector start = mins + position;
			Vector end   = maxs + position;
			debugoverlay->AddLineOverlay(start, end, r, g, b, noDepthTest, flDuration);
			
			start.x += (maxs.x - mins.x);
			end.x   -= (maxs.x - mins.x);
			debugoverlay->AddLineOverlay(start, end, r, g, b, noDepthTest, flDuration);
			
			start.y += (maxs.y - mins.y);
			end.y   -= (maxs.y - mins.y);
			debugoverlay->AddLineOverlay(start, end, r, g, b, noDepthTest, flDuration);
			
			start.x -= (maxs.x - mins.x);
			end.x   += (maxs.x - mins.x);
			debugoverlay->AddLineOverlay(start, end, r, g, b, noDepthTest, flDuration);
		}
	}
	
	void Cross3D_size(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		float size; _float32(&size, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Cross3D_size] <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				size,
				r, g, b,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddLineOverlay(position + Vector(size, 0.0f, 0.0f), position - Vector(size, 0.0f, 0.0f), r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position + Vector(0.0f, size, 0.0f), position - Vector(0.0f, size, 0.0f), r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position + Vector(0.0f, 0.0f, size), position - Vector(0.0f, 0.0f, size), r, g, b, noDepthTest, flDuration);
		}
	}
	
	void Cross3DOriented_ang(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		QAngle angles;   msg.ReadBitAngles(angles);
		float size; _float32(&size, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Cross3DOriented_ang] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				angles.x, angles.y, angles.z,
				size,
				r, g, b,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector forward, right, up;
			AngleVectors(angles, &forward, &right, &up);
			
			forward *= size;
			right   *= size;
			up      *= size;
			
			debugoverlay->AddLineOverlay(position + right,   position - right,   r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position + forward, position - forward, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position + up,      position - up,      r, g, b, noDepthTest, flDuration);
		}
	}
	
	void Do_HorzArrow(const Vector& startPos, const Vector& endPos, float width, int r, int g, int b, int a, bool noDepthTest, float flDuration)
	{
		Vector lineDir = (endPos - startPos);
		VectorNormalize(lineDir);
		Vector upVec = Vector(0.0f, 0.0f, 1.0f);
		Vector sideDir;
		float radius = width / 2.0f;
		
		CrossProduct(lineDir, upVec, sideDir);
		
		Vector p1 = startPos - (sideDir * radius);
		Vector p2 = endPos - (lineDir * width) - (sideDir * radius);
		Vector p3 = endPos - (lineDir * width) - (sideDir * width);
		Vector p4 = endPos;
		Vector p5 = endPos - (lineDir * width) + (sideDir * width);
		Vector p6 = endPos - (lineDir * width) + (sideDir * radius);
		Vector p7 = startPos + (sideDir * radius);
		
		debugoverlay->AddLineOverlay(p1, p2, r, g, b, noDepthTest, flDuration);
		debugoverlay->AddLineOverlay(p2, p3, r, g, b, noDepthTest, flDuration);
		debugoverlay->AddLineOverlay(p3, p4, r, g, b, noDepthTest, flDuration);
		debugoverlay->AddLineOverlay(p4, p5, r, g, b, noDepthTest, flDuration);
		debugoverlay->AddLineOverlay(p5, p6, r, g, b, noDepthTest, flDuration);
		debugoverlay->AddLineOverlay(p6, p7, r, g, b, noDepthTest, flDuration);
		
		if (a > 0) {
			debugoverlay->AddTriangleOverlay(p5, p4, p3, r, g, b, a, noDepthTest, flDuration);
			debugoverlay->AddTriangleOverlay(p1, p7, p6, r, g, b, a, noDepthTest, flDuration);
			debugoverlay->AddTriangleOverlay(p6, p2, p1, r, g, b, a, noDepthTest, flDuration);
			
			debugoverlay->AddTriangleOverlay(p3, p4, p5, r, g, b, a, noDepthTest, flDuration);
			debugoverlay->AddTriangleOverlay(p6, p7, p1, r, g, b, a, noDepthTest, flDuration);
			debugoverlay->AddTriangleOverlay(p1, p2, p6, r, g, b, a, noDepthTest, flDuration);
		}
	}
	
	void HorzArrow(bf_read& msg)
	{
		Vector startPos; msg.ReadBitVec3Coord(startPos);
		Vector endPos;   msg.ReadBitVec3Coord(endPos);
		float width; _float32(&width, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[HorzArrow] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %4.1f %02x%02x%02x%02x %d %.2f\n",
				startPos.x, startPos.y, startPos.z,
				endPos.x, endPos.y, endPos.z,
				width,
				r, g, b, a,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Do_HorzArrow(startPos, endPos, width, r, g, b, a, noDepthTest, flDuration);
		}
	}
	
	void YawArrow(bf_read& msg)
	{
		Vector startPos; msg.ReadBitVec3Coord(startPos);
		float yaw; _float32(&yaw, msg.ReadWord());
		float length; _float32(&length, msg.ReadWord());
		float width; _float32(&width, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[YawArrow] <%+4.0f %+4.0f %+4.0f> %5.1f %5.1f %4.1f %02x%02x%02x%02x %d %.2f\n",
				startPos.x, startPos.y, startPos.z,
				yaw,
				length,
				width,
				r, g, b, a,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Do_HorzArrow(startPos, startPos + (UTIL_YawToVector(yaw) * length), width, r, g, b, a, noDepthTest, flDuration);
		}
	}
	
	void VertArrow(bf_read& msg)
	{
		Vector startPos; msg.ReadBitVec3Coord(startPos);
		Vector endPos;   msg.ReadBitVec3Coord(endPos);
		float width; _float32(&width, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[VertArrow] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %4.1f %02x%02x%02x%02x %d %.2f\n",
				startPos.x, startPos.y, startPos.z,
				endPos.x, endPos.y, endPos.z,
				width,
				r, g, b, a,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector lineDir = (endPos - startPos);
			VectorNormalize(lineDir);
			Vector upVec;
			Vector sideDir;
			float radius = width / 2.0f;
			
			VectorVectors(lineDir, sideDir, upVec);
			
			Vector p1 = startPos - (upVec * radius);
			Vector p2 = endPos - (lineDir * width) - (upVec * radius);
			Vector p3 = endPos - (lineDir * width) - (upVec * width);
			Vector p4 = endPos;
			Vector p5 = endPos - (lineDir * width) + (upVec * width);
			Vector p6 = endPos - (lineDir * width) + (upVec * radius);
			Vector p7 = startPos + (upVec * radius);
			
			debugoverlay->AddLineOverlay(p1, p2, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(p2, p3, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(p3, p4, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(p4, p5, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(p5, p6, r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(p6, p7, r, g, b, noDepthTest, flDuration);
			
			if (a > 0) {
				debugoverlay->AddTriangleOverlay(p5, p4, p3, r, g, b, a, noDepthTest, flDuration);
				debugoverlay->AddTriangleOverlay(p1, p7, p6, r, g, b, a, noDepthTest, flDuration);
				debugoverlay->AddTriangleOverlay(p6, p2, p1, r, g, b, a, noDepthTest, flDuration);
				
				debugoverlay->AddTriangleOverlay(p3, p4, p5, r, g, b, a, noDepthTest, flDuration);
				debugoverlay->AddTriangleOverlay(p6, p7, p1, r, g, b, a, noDepthTest, flDuration);
				debugoverlay->AddTriangleOverlay(p1, p2, p6, r, g, b, a, noDepthTest, flDuration);
			}
		}
	}
	
	void Axis(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		QAngle angles;   msg.ReadBitAngles(angles);
		float size; _float32(&size, msg.ReadWord());
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Axis] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %4.1f %d %.2f\n",
				position.x, position.y, position.z,
				angles.x, angles.y, angles.z,
				size,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector xvec, yvec, zvec;
			AngleVectors(angles, &xvec, &yvec, &zvec);
			
			xvec = position + (size * xvec);
			yvec = position - (size * yvec);
			zvec = position + (size * zvec);
			
			debugoverlay->AddLineOverlay(position, xvec, 255, 0, 0, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position, yvec, 0, 255, 0, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(position, zvec, 0, 0, 255, noDepthTest, flDuration);
		}
	}
	
	void Sphere(bf_read& msg)
	{
		Vector center; msg.ReadBitVec3Coord(center);
		float radius; _float32(&radius, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Sphere] <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x %d %.2f\n",
				center.x, center.y, center.z,
				radius,
				r, g, b,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			CDebugOverlay::AddSphereOverlay(center, radius, cvar_sphere_ntheta.GetInt(), cvar_sphere_nphi.GetInt(),
				r, g, b, 0x00, flDuration);
		}
		
#if 0
		if (debugoverlay != nullptr) {
			Vector edge, lastEdge;
			
			float axisSize = radius;
			debugoverlay->AddLineOverlay(center + Vector(0.0f, 0.0f, -axisSize), center + Vector(0.0f, 0.0f, axisSize), r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(center + Vector(0.0f, -axisSize, 0.0f), center + Vector(0.0f, axisSize, 0.0f), r, g, b, noDepthTest, flDuration);
			debugoverlay->AddLineOverlay(center + Vector(-axisSize, 0.0f, 0.0f), center + Vector(axisSize, 0.0f, 0.0f), r, g, b, noDepthTest, flDuration);
			
			for (float angle = 0.0f; angle <= 360.0f; angle += 22.5f) {
				edge.x = center.x + (radius * cosf(angle / 180.0f * M_PI));
				edge.y = center.y;
				edge.z = center.z + (radius * sinf(angle / 180.0f * M_PI));
				
				if (angle != 0.0f) debugoverlay->AddLineOverlay(edge, lastEdge, r, g, b, noDepthTest, flDuration);
				
				lastEdge = edge;
			}
			
			for (float angle = 0.0f; angle <= 360.0f; angle += 22.5f) {
				edge.x = center.x;
				edge.y = center.y + (radius * cosf(angle / 180.0f * M_PI));
				edge.z = center.z + (radius * sinf(angle / 180.0f * M_PI));
				
				if (angle != 0.0f) debugoverlay->AddLineOverlay(edge, lastEdge, r, g, b, noDepthTest, flDuration);
				
				lastEdge = edge;
			}
			
			for (float angle = 0.0f; angle <= 360.0f; angle += 22.5f) {
				edge.x = center.x + (radius * cosf(angle / 180.0f * M_PI));
				edge.y = center.y + (radius * sinf(angle / 180.0f * M_PI));
				edge.z = center.z;
				
				if (angle != 0.0f) debugoverlay->AddLineOverlay(edge, lastEdge, r, g, b, noDepthTest, flDuration);
				
				lastEdge = edge;
			}
		}
#endif
	}
	
	void Do_Circle(const Vector& position, const Vector& xAxis, const Vector& yAxis, float radius, int r, int g, int b, int a, bool bNoDepthTest, float flDuration)
	{
		constexpr int nSegments = 16;
		constexpr float flRadStep = (M_PI * 2.0f) / (float)nSegments;
		
		Vector vecLastPosition;
		
		Vector vecStart = position + (radius * xAxis);
		Vector vecPosition = vecStart;
		
		for (int i = 1; i <= nSegments; ++i) {
			vecLastPosition = vecPosition;
			
			float flSin, flCos;
			SinCos(i * flRadStep, &flSin, &flCos);
			vecPosition = position + (radius * flCos * xAxis) + (radius * flSin * yAxis);
			
			debugoverlay->AddLineOverlay(vecLastPosition, vecPosition, r, g, b, bNoDepthTest, flDuration);
			
			if (a > 0 && i > 1) {
				debugoverlay->AddTriangleOverlay(vecStart, vecLastPosition, vecPosition, r, g, b, a, bNoDepthTest, flDuration);
				debugoverlay->AddTriangleOverlay(vecPosition, vecLastPosition, vecStart, r, g, b, a, bNoDepthTest, flDuration);
			}
		}
	}
	
	void Circle_ang(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		QAngle angles;   msg.ReadBitAngles(angles);
		float radius; _float32(&radius, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool bNoDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Circle_ang] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				angles.x, angles.y, angles.z,
				radius,
				r, g, b, a,
				bNoDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			matrix3x4_t xform;
			AngleMatrix(angles, position, xform);
			Vector xAxis, yAxis;
			MatrixGetColumn(xform, 2, xAxis);
			MatrixGetColumn(xform, 1, yAxis);
			
			Do_Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
		}
	}
	
	void Circle_axes(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		Vector xAxis;    msg.ReadBitVec3Coord(xAxis);
		Vector yAxis;    msg.ReadBitVec3Coord(yAxis);
		float radius; _float32(&radius, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool bNoDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Circle_axes] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				xAxis.x, xAxis.y, xAxis.z,
				yAxis.x, yAxis.y, yAxis.z,
				radius,
				r, g, b, a,
				bNoDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Do_Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
		}
	}
	
	void Sphere_ang(bf_read& msg)
	{
		Vector position; msg.ReadBitVec3Coord(position);
		QAngle angles;   msg.ReadBitAngles(angles);
		float radius; _float32(&radius, msg.ReadWord());
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool bNoDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[Sphere_ang] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %5.1f %02x%02x%02x%02x %d %.2f\n",
				position.x, position.y, position.z,
				angles.x, angles.y, angles.z,
				radius,
				r, g, b, a,
				bNoDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			CDebugOverlay::AddSphereOverlay(position, radius, cvar_sphere_ntheta.GetInt(), cvar_sphere_nphi.GetInt(),
				r, g, b, a, flDuration);
		}
		
#if 0
		if (debugoverlay != nullptr) {
			matrix3x4_t xform;
			AngleMatrix(angles, position, xform);
			Vector xAxis, yAxis, zAxis;
			MatrixGetColumn(xform, 0, xAxis);
			MatrixGetColumn(xform, 1, yAxis);
			MatrixGetColumn(xform, 2, zAxis);
			
			Do_Circle(position, xAxis, yAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
			Do_Circle(position, yAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
			Do_Circle(position, xAxis, zAxis, radius, r, g, b, a, bNoDepthTest, flDuration);
		}
#endif
	}
	
	
	void Clear(bf_read& msg)
	{
		if (cvar_trace.GetBool()) {
			DevMsg("[Clear]\n");
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->ClearAllOverlays();
		}
	}
	
	void LineAlpha(bf_read& msg)
	{
		Vector origin; msg.ReadBitVec3Coord(origin);
		Vector target; msg.ReadBitVec3Coord(target);
		int r = msg.ReadByte();
		int g = msg.ReadByte();
		int b = msg.ReadByte();
		int a = msg.ReadByte();
		bool noDepthTest = !!msg.ReadOneBit();
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[LineAlpha] <%+4.0f %+4.0f %+4.0f> <%+4.0f %+4.0f %+4.0f> %02x%02x%02x%02x %d %.2f\n",
				origin.x, origin.y, origin.z,
				target.x, target.y, target.z,
				r, g, b, a,
				noDepthTest,
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			debugoverlay->AddLineOverlayAlpha(origin, target, r, g, b, a, noDepthTest, flDuration);
		}
	}
	
	void ScreenRect(bf_read& msg)
	{
		float xFrom; _float32(&xFrom, msg.ReadWord());
		float yFrom; _float32(&yFrom, msg.ReadWord());
		float xTo;   _float32(&xTo,   msg.ReadWord());
		float yTo;   _float32(&yTo,   msg.ReadWord());
		Color cFill; cFill.SetRawColor((int)msg.ReadLong());
		Color cEdge; cEdge.SetRawColor((int)msg.ReadLong());
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[ScreenRect] <%5.1f %5.1f> <%5.1f %5.1f> %02x%02x%02x%02x %02x%02x%02x%02x %.2f\n",
				xFrom, xFrom,
				xTo,   yTo,
				cFill[0], cFill[1], cFill[2], cFill[3],
				cEdge[0], cEdge[1], cEdge[2], cEdge[3],
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector origin;
			Vector mins;
			Vector maxs;
			QAngle angles;
			int r;
			int g;
			int b;
			int a;
			
			static constexpr uint32_t nan1  = 0x7fc00001;
			static constexpr uint32_t nan2  = 0x7fc00002;
			static constexpr uint32_t nan3  = 0x7fc00003;
			static constexpr uint32_t nan4  = 0x7fc00004;
			static constexpr uint32_t nanID = 0x7fc00100;
			
			memcpy(&origin.x, &nan1, sizeof(float));
			memcpy(&origin.y, &nan2, sizeof(float));
			memcpy(&origin.z, &nan3, sizeof(float));
			mins.x = xFrom;
			mins.y = yFrom;
			memcpy(&mins.z, &nan4, sizeof(float));
			maxs.x = xTo;
			maxs.y = yTo;
			memcpy(&maxs.z, &nanID, sizeof(float));
			angles.x = -0.0f;
			angles.y = -0.0f;
			angles.z = -0.0f;
			memcpy(&r, &cFill, sizeof(int));
			memcpy(&g, &cEdge, sizeof(int));
			b = std::numeric_limits<int>::max();
			a = std::numeric_limits<int>::min();
			
			DevMsg("<<< RecvScreenRect [xFrom %.3f] [yFrom %.3f] [xTo %.3f] [yTo %.3f] [cFill %02x%02x%02x%02x] [cEdge %02x%02x%02x%02x]\n",
				xFrom, yFrom, xTo, yTo,
				cFill[0], cFill[1], cFill[2], cFill[3],
				cEdge[0], cEdge[1], cEdge[2], cEdge[3]);
			DevMsg(
				"<<< %08x %08x %08x %08x %08x %08x %08x %08x\n"
				"<<< %08x %08x %08x %08x %08x %08x %08x %08x\n",
				*(uint32_t *)((uintptr_t)&origin + 0x00),
				*(uint32_t *)((uintptr_t)&origin + 0x04),
				*(uint32_t *)((uintptr_t)&origin + 0x08),
				*(uint32_t *)((uintptr_t)&mins   + 0x00),
				*(uint32_t *)((uintptr_t)&mins   + 0x04),
				*(uint32_t *)((uintptr_t)&mins   + 0x08),
				*(uint32_t *)((uintptr_t)&maxs   + 0x00),
				*(uint32_t *)((uintptr_t)&maxs   + 0x04),
				*(uint32_t *)((uintptr_t)&maxs   + 0x08),
				*(uint32_t *)((uintptr_t)&angles + 0x00),
				*(uint32_t *)((uintptr_t)&angles + 0x04),
				*(uint32_t *)((uintptr_t)&angles + 0x08),
				*(uint32_t *)((uintptr_t)&r      + 0x00),
				*(uint32_t *)((uintptr_t)&g      + 0x00),
				*(uint32_t *)((uintptr_t)&b      + 0x00),
				*(uint32_t *)((uintptr_t)&a      + 0x00));
			
			debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	
	void ScreenLine(bf_read& msg, int colors)
	{
		float xFrom; _float32(&xFrom, msg.ReadWord());
		float yFrom; _float32(&yFrom, msg.ReadWord());
		float xTo;   _float32(&xTo,   msg.ReadWord());
		float yTo;   _float32(&yTo,   msg.ReadWord());
		Color cFrom; cFrom.SetRawColor((int)msg.ReadLong());
		Color cTo;   cTo.SetRawColor((colors == 1 ? cFrom.GetRawColor() : (int)msg.ReadLong()));
		float flDuration; _float32(&flDuration, msg.ReadWord());
		
		if (cvar_trace.GetBool()) {
			DevMsg("[ScreenLine] <%5.1f %5.1f> <%5.1f %5.1f> %02x%02x%02x%02x %02x%02x%02x%02x %.2f\n",
				xFrom, xFrom,
				xTo,   yTo,
				cFrom[0], cFrom[1], cFrom[2], cFrom[3],
				cTo  [0], cTo  [1], cTo  [2], cTo  [3],
				flDuration);
		}
		
		if (debugoverlay != nullptr) {
			Vector origin;
			Vector mins;
			Vector maxs;
			QAngle angles;
			int r;
			int g;
			int b;
			int a;
			
			static constexpr uint32_t nan1  = 0x7fc00001;
			static constexpr uint32_t nan2  = 0x7fc00002;
			static constexpr uint32_t nan3  = 0x7fc00003;
			static constexpr uint32_t nan4  = 0x7fc00004;
			static constexpr uint32_t nanID = 0x7fc00200;
			
			memcpy(&origin.x, &nan1, sizeof(float));
			memcpy(&origin.y, &nan2, sizeof(float));
			memcpy(&origin.z, &nan3, sizeof(float));
			mins.x = xFrom;
			mins.y = yFrom;
			memcpy(&mins.z, &nan4, sizeof(float));
			maxs.x = xTo;
			maxs.y = yTo;
			memcpy(&maxs.z, &nanID, sizeof(float));
			angles.x = -0.0f;
			angles.y = -0.0f;
			angles.z = -0.0f;
			memcpy(&r, &cFrom, sizeof(int));
			memcpy(&g, &cTo,   sizeof(int));
			b = std::numeric_limits<int>::max();
			a = std::numeric_limits<int>::min();
			
			DevMsg("<<< RecvScreenLine [xFrom %.3f] [yFrom %.3f] [xTo %.3f] [yTo %.3f] [cFrom %02x%02x%02x%02x] [cTo %02x%02x%02x%02x]\n",
				xFrom, yFrom, xTo, yTo,
				cFrom[0], cFrom[1], cFrom[2], cFrom[3],
				cTo  [0], cTo  [1], cTo  [2], cTo  [3]);
			DevMsg(
				"<<< %08x %08x %08x %08x %08x %08x %08x %08x\n"
				"<<< %08x %08x %08x %08x %08x %08x %08x %08x\n",
				*(uint32_t *)((uintptr_t)&origin + 0x00),
				*(uint32_t *)((uintptr_t)&origin + 0x04),
				*(uint32_t *)((uintptr_t)&origin + 0x08),
				*(uint32_t *)((uintptr_t)&mins   + 0x00),
				*(uint32_t *)((uintptr_t)&mins   + 0x04),
				*(uint32_t *)((uintptr_t)&mins   + 0x08),
				*(uint32_t *)((uintptr_t)&maxs   + 0x00),
				*(uint32_t *)((uintptr_t)&maxs   + 0x04),
				*(uint32_t *)((uintptr_t)&maxs   + 0x08),
				*(uint32_t *)((uintptr_t)&angles + 0x00),
				*(uint32_t *)((uintptr_t)&angles + 0x04),
				*(uint32_t *)((uintptr_t)&angles + 0x08),
				*(uint32_t *)((uintptr_t)&r      + 0x00),
				*(uint32_t *)((uintptr_t)&g      + 0x00),
				*(uint32_t *)((uintptr_t)&b      + 0x00),
				*(uint32_t *)((uintptr_t)&a      + 0x00));
			
			debugoverlay->AddBoxOverlay(origin, mins, maxs, angles, r, g, b, a, flDuration);
		}
	}
	
	
	void BandwidthTest(bf_read& msg)
	{
		int bits = msg.ReadWord();
		
		if (cvar_trace.GetBool()) {
			DevMsg("[BandwidthTest] %d bits\n",
				bits);
		}
		
		msg.SeekRelative(bits);
	}
	
	
	void Hook_Overlays(bf_read& msg)
	{
//		while (msg.GetNumBitsLeft() > 0) {
			switch (msg.ReadUBitLong(OVERLAY_TYPE_BITS)) {
				
			case OV_BOX:                     Box(msg);                  break;
			case OV_BOX_ANGLES:              BoxAngles(msg);            break;
			case OV_SWEPT_BOX:               SweptBox(msg);             break;
			case OV_LINE:                    Line(msg);                 break;
			case OV_TRIANGLE:                Triangle(msg);             break;
			case OV_ENTITY_TEXT:             EntityText(msg);           break;
			case OV_ENTITY_TEXT_AT_POSITION: EntityTextAtPosition(msg); break;
			case OV_GRID:                    Grid(msg);                 break;
			case OV_TEXT:                    Text(msg);                 break;
			case OV_SCREEN_TEXT:             ScreenText(msg);           break;
			case OV_CROSS3D_EXT:             Cross3D_ext(msg);          break;
			case OV_CROSS3D_SIZE:            Cross3D_size(msg);         break;
			case OV_CROSS3D_ORIENTED_ANG:    Cross3DOriented_ang(msg);  break;
			case OV_HORZ_ARROW:              HorzArrow(msg);            break;
			case OV_YAW_ARROW:               YawArrow(msg);             break;
			case OV_VERT_ARROW:              VertArrow(msg);            break;
			case OV_AXIS:                    Axis(msg);                 break;
			case OV_SPHERE:                  Sphere(msg);               break;
			case OV_CIRCLE_ANG:              Circle_ang(msg);           break;
			case OV_CIRCLE_AXES:             Circle_axes(msg);          break;
			case OV_SPHERE_ANG:              Sphere_ang(msg);           break;
			case OV_CLEAR:                   Clear(msg);                break;
			case OV_LINE_ALPHA:              LineAlpha(msg);            break;
			case OV_SCREEN_RECT:             ScreenRect(msg);           break;
			case OV_SCREEN_LINE_2COLOR:      ScreenLine(msg, 2);        break;
			case OV_SCREEN_LINE_1COLOR:      ScreenLine(msg, 1);        break;
			case OV_BANDWIDTH_TEST:          BandwidthTest(msg);        break;
				
			default:
				Warning("Unknown overlay type\n");
				break;
			}
//		}
	}
	
	
	struct DelayedPacket
	{
		DelayedPacket(int bits, int tick, const uint8_t *src) :
			bits(bits), tick(tick)
		{
			this->ptr = new uint8_t[BitByte(bits)];
			memcpy(this->ptr, src, BitByte(bits));
		}
		~DelayedPacket()
		{
			if (this->ptr != nullptr) {
				delete[] this->ptr;
				this->ptr = nullptr;
			}
		}
		
		DelayedPacket(const DelayedPacket&) = delete;
		
		int bits;
		int tick;
		uint8_t *ptr = nullptr;
	};
	
	std::list<DelayedPacket> queued_packets;
	
	
	void ProcessPacket(const DelayedPacket& packet)
	{
		bf_read src(packet.ptr, BitByte(packet.bits), packet.bits);
		
		while (src.GetNumBitsRead() < packet.bits) {
			Hook_Overlays(src);
		}
		
	//	DevMsg("POP:  %d bits @ tick %d\n", packet.bits, packet.tick);
	}
	
	void ProcessQueuedPackets()
	{
		for (auto it = queued_packets.begin(); it != queued_packets.end(); ) {
			const auto& packet = *it;
			
			if (packet.tick <= enginetools->ClientTick()) {
				ProcessPacket(packet);
				it = queued_packets.erase(it);
			} else {
				++it;
			}
		}
	}
	
	
	FirehoseRecv *recv = nullptr;
	void ReceivePackets()
	{
		static std::vector<uint8_t> buffer;
		buffer.resize(65536);
		
		size_t len;
		while ((len = recv->Recv(buffer.size(), buffer.data())) != 0) {
		//	DevMsg("Received %u bytes\n", len);
			
			bf_read hdr(buffer.data(), buffer.size());
			
			uint32_t bits = hdr.ReadUBitLong(32);
			uint32_t tick = hdr.ReadUBitLong(32);
			
			queued_packets.emplace_back(bits, tick, buffer.data() + 8);
			
		//	DevMsg("PUSH: %d bits @ tick %d\n", bits, tick);
		}
	}
	
	
	DETOUR_DECL_STATIC(void, CDebugOverlay_DrawAllOverlays)
	{
		ReceivePackets();
		ProcessQueuedPackets();
		
		DETOUR_STATIC_CALL(CDebugOverlay_DrawAllOverlays)();
		
#if 0
		static std::vector<uint8_t> packet;
		packet.resize(65536);
		
		size_t len;
		while ((len = recv->Recv(packet.size(), packet.data())) != 0) {
		//	DevMsg("Received %u bytes\n", len);
			
			bf_read src(packet.data(), packet.size());
			uint32_t bits = src.ReadUBitLong(32);
			
			float when = src.ReadFloat();
			
			while (src.GetNumBitsRead() < bits) {
				Hook_Overlays(src);
			}
		}
#endif
	}
	
	
	// stale text overlays are cleared:
	// CDebugOverlay::ClearDeadOverlays
	// via CIVDebugOverlay::ClearDeadOverlays
	// via IVDebugOverlay::ClearDeadOverlays
	// via CDebugOverlay::Paint
	
	// stale non-text overlays are cleared:
	// CDebugOverlay::DrawAllOverlays
	// via CDebugOverlay::Draw3DOverlays
	// via CVRenderView::Draw3DDebugOverlays
	
	
	// the real solution:
	// may as well keep receiving when we do, but
	// what we really need to do is
	// (a) not clear dead overlays when painting
	// (b) do clear dead overlays when receiving
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Overlay_Recv")
		{
			MOD_ADD_DETOUR_STATIC(CDebugOverlay_DrawAllOverlays, "[client] CDebugOverlay::DrawAllOverlays");
			
		//	MOD_ADD_DETOUR_MEMBER(IVDebugOverlay_ClearDeadOverlays, "IVDebugOverlay::ClearDeadOverlays");
		}
		
		virtual void OnEnable() override
		{
			recv = new FirehoseRecv(OVERLAY_PORT);
			
#if 0
			if (usermessages->LookupUserMessage("Overlays") == -1) {
				usermessages->Register("Overlays", -1);
			}
			
			usermessages->HookMessage("Overlays", &Hook_Overlays);
#endif
		}
		virtual void OnDisable() override
		{
			delete recv;
			recv = nullptr;
			
#if 0
			usermessages->UnHookMessage("Overlays", &Hook_Overlays);
#endif
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_overlay_recv", "0", FCVAR_NOTIFY,
		"Utility: overlay forwarding: client receive",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
